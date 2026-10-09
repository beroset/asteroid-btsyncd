/*
 * Copyright (C) 2026 - The asteroid-btsyncd contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "hidkeyboardservice.h"

#include <QDebug>

#include "characteristic.h"
#include "descriptor.h"
#include "notifyingcharacteristic.h"

namespace {

inline constexpr char HID_SERVICE_UUID[] = "00001812-0000-1000-8000-00805f9b34fb";
inline constexpr char HID_INFO_UUID[] = "00002a4a-0000-1000-8000-00805f9b34fb";
inline constexpr char HID_REPORT_MAP_UUID[] = "00002a4b-0000-1000-8000-00805f9b34fb";
inline constexpr char HID_CONTROL_POINT_UUID[] = "00002a4c-0000-1000-8000-00805f9b34fb";
inline constexpr char HID_REPORT_UUID[] = "00002a4d-0000-1000-8000-00805f9b34fb";
inline constexpr char HID_PROTOCOL_MODE_UUID[] = "00002a4e-0000-1000-8000-00805f9b34fb";
inline constexpr char BOOT_KEYBOARD_INPUT_UUID[] = "00002a22-0000-1000-8000-00805f9b34fb";
inline constexpr char BOOT_KEYBOARD_OUTPUT_UUID[] = "00002a32-0000-1000-8000-00805f9b34fb";
inline constexpr char REPORT_REFERENCE_UUID[] = "00002908-0000-1000-8000-00805f9b34fb";

const QByteArray HID_REPORT_MAP = QByteArray::fromHex(
    "05010906a1018501050719e029e715002501750195088102"
    "95017508810195067508150025650507190029658100"
    "85020508190129059102950175039101c0");

class ValueCharacteristic final : public Characteristic
{
public:
    ValueCharacteristic(QDBusConnection bus, unsigned int index, const QString &uuid,
                        const QStringList &flags, Service *service, QByteArray value)
        : Characteristic(bus, index, uuid, flags, service, service), mValue(std::move(value))
    {
    }

    QByteArray ReadValue(QVariantMap) override
    {
        return mValue;
    }

private:
    QByteArray mValue;
};

class ReportReference final : public Descriptor
{
public:
    ReportReference(QDBusConnection bus, Characteristic *characteristic,
                    unsigned int reportId, unsigned int reportType)
        : Descriptor(bus, 0, {"read"}, characteristic, REPORT_REFERENCE_UUID, characteristic)
        , mValue{static_cast<char>(reportId), static_cast<char>(reportType)}
    {
    }

    QByteArray ReadValue(QVariantMap) override
    {
        return mValue;
    }

private:
    QByteArray mValue;
};

class WritableCharacteristic final : public Characteristic
{
public:
    using WriteHandler = std::function<void(const QByteArray &)>;

    WritableCharacteristic(QDBusConnection bus, unsigned int index, const QString &uuid,
                           const QStringList &flags, Service *service, WriteHandler handler)
        : Characteristic(bus, index, uuid, flags, service, service), mHandler(std::move(handler))
    {
    }

    void WriteValue(QByteArray value, QVariantMap) override
    {
        mHandler(value);
    }

private:
    WriteHandler mHandler;
};

} // namespace

class HidKeyboardService::InputReport final : public NotifyingCharacteristic
{
public:
    InputReport(QDBusConnection bus, unsigned int index, const QString &uuid, Service *service,
                bool reportProtocol)
        : NotifyingCharacteristic(bus, index, uuid,
                                  {"encrypt-authenticated-read", "encrypt-authenticated-notify"},
                                  service, QByteArray(8, 0))
        , mReportProtocol(reportProtocol)
    {
    }

    void publish(const QByteArray &state, bool suspended)
    {
        QByteArray value = suspended ? QByteArray(8, 0) : state;
        if (mReportProtocol)
            value.prepend(static_cast<char>(1));
        setValue(value);
    }

private:
    bool mReportProtocol;
};

HidKeyboardService::HidKeyboardService(int index, QDBusConnection bus, QObject *parent)
    : Service(bus, index, HID_SERVICE_UUID, parent)
{
    auto *info = new ValueCharacteristic(bus, 0, HID_INFO_UUID, {"encrypt-authenticated-read"},
                                         this, QByteArray::fromHex("11010002"));
    addCharacteristic(info);

    addCharacteristic(new ValueCharacteristic(bus, 1, HID_REPORT_MAP_UUID,
                                               {"encrypt-authenticated-read"}, this,
                                               HID_REPORT_MAP));

    addCharacteristic(new WritableCharacteristic(
        bus, 2, HID_CONTROL_POINT_UUID, {"encrypt-authenticated-write", "write-without-response"},
        this, [this](const QByteArray &value) {
            if (value.size() != 1 || static_cast<unsigned char>(value.at(0)) > 1) {
                qWarning() << "Ignoring malformed HID Control Point write";
                return;
            }
            setSuspended(value.at(0) == 0);
        }));

    addCharacteristic(new WritableCharacteristic(
        bus, 3, HID_PROTOCOL_MODE_UUID,
        {"encrypt-authenticated-read", "encrypt-authenticated-write", "write", "write-without-response"},
        this, [this](const QByteArray &value) {
            if (value.size() != 1 || static_cast<unsigned char>(value.at(0)) > 1) {
                qWarning() << "Ignoring invalid HID Protocol Mode";
                return;
            }
            setBootProtocol(value.at(0) == 0);
        }));

    mReportInput = new InputReport(bus, 4, HID_REPORT_UUID, this, true);
    addCharacteristic(mReportInput);
    mReportInput->addDescriptor(new ReportReference(bus, mReportInput, 1, 1));

    auto *reportOutput = new WritableCharacteristic(
        bus, 5, HID_REPORT_UUID,
        {"encrypt-authenticated-read", "encrypt-authenticated-write", "write", "write-without-response"},
        this, [this](const QByteArray &value) {
            if (value.size() != 2 || static_cast<unsigned char>(value.at(0)) != 2
                || (static_cast<unsigned char>(value.at(1)) & 0xe0) != 0) {
                qWarning() << "Ignoring malformed keyboard output report";
                return;
            }
            emit outputReportChanged(static_cast<unsigned char>(value.at(1)));
        });
    reportOutput->addDescriptor(new ReportReference(bus, reportOutput, 2, 2));
    addCharacteristic(reportOutput);

    mBootInput = new InputReport(bus, 6, BOOT_KEYBOARD_INPUT_UUID, this, false);
    addCharacteristic(mBootInput);

    addCharacteristic(new WritableCharacteristic(
        bus, 7, BOOT_KEYBOARD_OUTPUT_UUID,
        {"encrypt-authenticated-write", "write", "write-without-response"}, this,
        [this](const QByteArray &value) {
            if (value.size() != 1 || (static_cast<unsigned char>(value.at(0)) & 0xe0) != 0) {
                qWarning() << "Ignoring malformed boot keyboard output report";
                return;
            }
            emit outputReportChanged(static_cast<unsigned char>(value.at(0)));
        }));

    updateInputReports();
}

bool HidKeyboardService::setKeyState(const QByteArray &state)
{
    if (state.size() != 8 || state.at(1) != 0) {
        qWarning() << "Rejecting keyboard state: expected an 8-byte boot keyboard report";
        return false;
    }

    for (int i = 2; i < state.size(); ++i) {
        const unsigned char usage = static_cast<unsigned char>(state.at(i));
        if (usage > 0x65) {
            qWarning() << "Rejecting keyboard state with invalid key usage" << usage;
            return false;
        }
    }

    mKeyState = state;
    updateInputReports();
    return true;
}

QByteArray HidKeyboardService::keyState() const
{
    return mKeyState;
}

void HidKeyboardService::setSuspended(bool suspended)
{
    if (mSuspended == suspended)
        return;
    mSuspended = suspended;
    updateInputReports();
}

bool HidKeyboardService::bootProtocol() const
{
    return mBootProtocol;
}

void HidKeyboardService::setBootProtocol(bool bootProtocol)
{
    if (mBootProtocol == bootProtocol)
        return;
    mBootProtocol = bootProtocol;
    updateInputReports();
}

void HidKeyboardService::updateInputReports()
{
    mReportInput->publish(mKeyState, mSuspended);
    mBootInput->publish(mKeyState, mSuspended);
}
