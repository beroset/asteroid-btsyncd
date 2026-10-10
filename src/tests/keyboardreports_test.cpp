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

#include "keyboardreports.h"
#include "hidkeyboardservice.h"
#include "notifyingcharacteristic.h"

#include <QtTest/QtTest>
#include <QDBusConnection>
#include <QSignalSpy>

class KeyboardReportsTest : public QObject
{
    Q_OBJECT

private slots:
    void keyStateValidation()
    {
        QVERIFY(KeyboardReports::isValidKeyState(QByteArray(KeyboardReports::KeyStateSize, 0)));
        QVERIFY(!KeyboardReports::isValidKeyState(QByteArray(KeyboardReports::KeyStateSize - 1, 0)));
        QVERIFY(!KeyboardReports::isValidKeyState(QByteArray(KeyboardReports::KeyStateSize + 1, 0)));

        QByteArray state(KeyboardReports::KeyStateSize, 0);
        state[KeyboardReports::KeyUsageOffset] = 0x04;
        QVERIFY(KeyboardReports::isValidKeyState(state));
        state[KeyboardReports::ReservedByteOffset] = 1;
        QVERIFY(!KeyboardReports::isValidKeyState(state));
        state[KeyboardReports::ReservedByteOffset] = 0;
        for (unsigned char invalidUsage = KeyboardReports::ErrorRollOverUsage;
             invalidUsage <= KeyboardReports::ErrorUndefinedUsage; ++invalidUsage) {
            state[KeyboardReports::KeyUsageOffset] = static_cast<char>(invalidUsage);
            QVERIFY(!KeyboardReports::isValidKeyState(state));
        }
        state[KeyboardReports::KeyUsageOffset] =
            static_cast<char>(KeyboardReports::MaxKeyUsage + 1);
        QVERIFY(!KeyboardReports::isValidKeyState(state));
    }

    void outputReportValidation()
    {
        QVERIFY(KeyboardReports::isValidReportOutput(QByteArray::fromHex("1f")));
        QVERIFY(!KeyboardReports::isValidReportOutput(QByteArray()));
        QVERIFY(!KeyboardReports::isValidReportOutput(QByteArray::fromHex("1f00")));
        QVERIFY(!KeyboardReports::isValidReportOutput(QByteArray::fromHex("e0")));
    }

    void bootOutputValidation()
    {
        QVERIFY(KeyboardReports::isValidBootOutput(QByteArray::fromHex("1f")));
        QVERIFY(!KeyboardReports::isValidBootOutput(QByteArray::fromHex("1f00")));
        QVERIFY(!KeyboardReports::isValidBootOutput(QByteArray::fromHex("e0")));
    }

    void controlPointValidation()
    {
        QVERIFY(KeyboardReports::isValidControlPoint(QByteArray::fromHex("00")));
        QVERIFY(KeyboardReports::isValidControlPoint(QByteArray::fromHex("01")));
        QVERIFY(!KeyboardReports::isValidControlPoint(QByteArray()));
        QVERIFY(!KeyboardReports::isValidControlPoint(QByteArray::fromHex("0000")));
        QVERIFY(!KeyboardReports::isValidControlPoint(QByteArray::fromHex("02")));
    }

    void protocolModeValidation()
    {
        QVERIFY(KeyboardReports::isValidProtocolMode(
            QByteArray(1, KeyboardReports::BootProtocolMode)));
        QVERIFY(KeyboardReports::isValidProtocolMode(
            QByteArray(1, KeyboardReports::ReportProtocolMode)));
        QVERIFY(!KeyboardReports::isValidProtocolMode(QByteArray()));
        QVERIFY(!KeyboardReports::isValidProtocolMode(QByteArray::fromHex("0100")));
        QVERIFY(!KeyboardReports::isValidProtocolMode(QByteArray::fromHex("02")));
    }

    void inputReportProtocolAndSuspendGating()
    {
        QByteArray state(KeyboardReports::KeyStateSize, 0);
        state[KeyboardReports::KeyUsageOffset] = 0x04;
        const QByteArray emptyState(KeyboardReports::KeyStateSize, 0);

        QCOMPARE(KeyboardReports::reportModeInputValue(state, false, false), state);
        QCOMPARE(KeyboardReports::reportModeInputValue(state, false, true), emptyState);
        QCOMPARE(KeyboardReports::reportModeInputValue(state, true, false), emptyState);
        QCOMPARE(KeyboardReports::bootModeInputValue(state, false, true), state);
        QCOMPARE(KeyboardReports::bootModeInputValue(state, false, false), emptyState);
        QCOMPARE(KeyboardReports::bootModeInputValue(state, true, true), emptyState);
    }

    void serviceWriteHandlersAndInputValues()
    {
        HidKeyboardService service(0, QDBusConnection(QStringLiteral("keyboardreports-test")));
        const QList<Characteristic *> characteristics = service.getCharacteristics();
        QCOMPARE(characteristics.size(), 8);
        auto *reportInput = qobject_cast<NotifyingCharacteristic *>(characteristics.at(4));
        auto *reportOutput = characteristics.at(5);
        auto *protocolMode = characteristics.at(3);
        auto *controlPoint = characteristics.at(2);
        QVERIFY(reportInput);

        qRegisterMetaType<quint8>();
        QSignalSpy outputSpy(&service, &HidKeyboardService::outputReportChanged);
        QVERIFY(outputSpy.isValid());
        reportOutput->WriteValue(QByteArray::fromHex("05"), QVariantMap{});
        QCOMPARE(outputSpy.count(), 1);
        QCOMPARE(outputSpy.takeFirst().at(0).toUInt(), 5U);
        reportOutput->WriteValue(QByteArray::fromHex("e0"), QVariantMap{});
        QCOMPARE(outputSpy.count(), 0);

        QByteArray state(KeyboardReports::KeyStateSize, 0);
        state[KeyboardReports::KeyUsageOffset] = 0x04;
        QVERIFY(service.setKeyState(state));
        QCOMPARE(reportInput->getValue(), state);

        protocolMode->WriteValue(
            QByteArray(1, KeyboardReports::BootProtocolMode), QVariantMap{});
        QCOMPARE(protocolMode->ReadValue(QVariantMap{}),
                 QByteArray(1, KeyboardReports::BootProtocolMode));
        QCOMPARE(reportInput->getValue(), QByteArray(KeyboardReports::KeyStateSize, 0));

        controlPoint->WriteValue(
            QByteArray(1, KeyboardReports::SuspendControlPointValue), QVariantMap{});
        QCOMPARE(characteristics.at(6)->ReadValue(QVariantMap{}),
                 QByteArray(KeyboardReports::KeyStateSize, 0));
        controlPoint->WriteValue(
            QByteArray(1, KeyboardReports::ExitSuspendControlPointValue), QVariantMap{});
        QCOMPARE(characteristics.at(6)->ReadValue(QVariantMap{}), state);
    }
};

QTEST_GUILESS_MAIN(KeyboardReportsTest)

#include "keyboardreports_test.moc"
