// Implements the Current Time Service as per: https://www.bluetooth.com/specifications/specs/cts-1-1/
#include "cts.h"

#include <QDebug>
#include <QDateTime>
#include <QTimeZone>

#include "gattbytes.h"
#include "settime.h"

CTS::CTS() : timeCharacteristic(CTS_CHARACTERISTIC_UUID) {
    connect(&timeCharacteristic, &RemoteCharacteristic::valueChanged,
            this, &CTS::onTimeValueChanged);
}

void CTS::search() {
    qDebug() << "CTS searching for characteristic";
    if (timeCharacteristic.find()) {
        qDebug() << "Current Time Characteristic found";
        timeCharacteristic.startNotify();

        QByteArray bytes = timeCharacteristic.readValue();
        if (!bytes.isEmpty())
            parseCurrentTime(bytes);
    }
}

void CTS::disconnect()
{
    timeCharacteristic.stopNotify();
}

void CTS::parseCurrentTime(QByteArray& bytes)
{
    if(bytes.size() != 10) {
        qWarning() << "Current time value is not 10 bytes long";
        return;
    }
    ushort year = GattBytes::readLittleEndian<ushort>(bytes, 0, 2).value_or(0);
    uint8_t month = bytes[2];
    uint8_t day = bytes[3];
    uint8_t hour = bytes[4];
    uint8_t minute = bytes[5];
    uint8_t second = bytes[6];
    // Bytes 7-9 (day of week, 1/256th second fractional time, adjust
    // reason) are part of the CTS spec's Exact Time 256 layout but are not
    // needed to set the system clock, so they are intentionally not read.

    QDateTime newTime(QDate(year, month, day), QTime(hour, minute, second));
    newTime.setTimeZone(QTimeZone::systemTimeZone());
    setSystemTime(newTime);
}

void CTS::onTimeValueChanged(const QByteArray &bytes)
{
    QByteArray mutableBytes = bytes;
    parseCurrentTime(mutableBytes);
}

#include "remotefeatureregistry.h"

REGISTER_REMOTE_FEATURE(CTS)
