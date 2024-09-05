#include "TimeService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyDescriptorData>
#include <QDateTime>
#include <QTimeZone>

#include <timed-qt5/wallclock>
#include <timed-qt5/interface>
#include <QDebug>

static const QBluetoothUuid TimeServiceUuid{QString{"00005071-0000-0000-0000-00A57E401D05"}};
static const QBluetoothUuid TimeUpdateUuid{QString{"00005001-0000-0000-0000-00A57E401D05"}};

TimeService::TimeService(BluetoothService &bluetoothService, QObject *parent) : QObject(parent) {
    QLowEnergyServiceData serviceData = createTimeServiceData();
    m_service = bluetoothService.addService(serviceData);
    connect(m_service, &QLowEnergyService::characteristicChanged, this, &TimeService::onCharacteristicWritten);
}

QLowEnergyService* TimeService::service() const {
    return m_service;
}

void TimeService::onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value) 
{
    int year = 1900 + (unsigned char) value[0];
    int month = 1+ (unsigned char) value[1];
    int day = (unsigned char) value[2];
    int hour = (unsigned char) value[3];
    int minute = (unsigned char) value[4];
    int second = (unsigned char) value[5];

    Maemo::Timed::WallClock::Settings s;
    QDateTime newTime(QDate(year, month, day), QTime(hour, minute, second));
    newTime.setTimeZone(QTimeZone::systemTimeZone());
    s.setTimeManual(newTime.toTime_t());

    Maemo::Timed::Interface timed;
    timed.wall_clock_settings_async(s);
}

QLowEnergyServiceData TimeService::createTimeServiceData() {
    QLowEnergyCharacteristicData timeUpdateData;
    timeUpdateData.setUuid(TimeUpdateUuid);
    timeUpdateData.setProperties(QLowEnergyCharacteristic::Write);
    timeUpdateData.setValue(QByteArray()); // empty value initially

    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(TimeServiceUuid);
    serviceData.addCharacteristic(timeUpdateData);

    return serviceData;
}

