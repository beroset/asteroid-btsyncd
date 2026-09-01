// Implements the Current Time Service as per: https://www.bluetooth.com/specifications/specs/cts-1-1/
#include "cts.h"

#include <QDebug>
#include <QDateTime>
#include <QTimeZone>

#include "settime.h"

CTS::CTS() : timeCharacteristic(CTS_CHARACTERISTIC_UUID) {
    connect(&timeCharacteristic, &RemoteCharacteristic::valueChanged,
            this, &CTS::onTimeValueChanged);
}

void CTS::searchForTimeCharacteristics() {
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
    ushort year = (bytes[1] << 8)  + bytes[0];
    uint8_t month = bytes[2];
    uint8_t day = bytes[3];
    uint8_t hour = bytes[4];
    uint8_t minute = bytes[5];
    uint8_t second = bytes[6];
    uint8_t day_of_week = bytes[7];
    uint8_t exact_time_256 = bytes[8];
    uint8_t adjust_reason = bytes[9];

    QDateTime newTime(QDate(year, month, day), QTime(hour, minute, second));
    newTime.setTimeZone(QTimeZone::systemTimeZone());
    setSystemTime(newTime);
}

void CTS::onTimeValueChanged(const QByteArray &bytes)
{
    QByteArray mutableBytes = bytes;
    parseCurrentTime(mutableBytes);
}
