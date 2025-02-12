#include "TimeService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyDescriptorData>
#include <QDateTime>
#include <QTimeZone>
#ifndef DESKTOP_VERSION
#include <timed-qt5/wallclock>
#include <timed-qt5/interface>
#endif
#include <QDebug>

static const QBluetoothUuid TimeServiceUuid{QString{"00005071-0000-0000-0000-00A57E401D05"}};
static const QBluetoothUuid TimeUpdateUuid{QString{"00005001-0000-0000-0000-00A57E401D05"}};

TimeService::TimeService(BluetoothService &bluetoothService, QObject *parent) : QObject(parent) {
    add(bluetoothService);
}

void TimeService::add(BluetoothService &bluetoothService)
{
    QLowEnergyServiceData serviceData = createTimeServiceData();
    m_service = bluetoothService.addService(serviceData);
    connect(m_service, &QLowEnergyService::characteristicChanged, this, &TimeService::onCharacteristicWritten);
}

void TimeService::remove()
{
    disconnect(m_service, nullptr, this, nullptr);
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
#ifdef DESKTOP_VERSION
    qDebug() << "Setting clock to" << year << "/" << month << "/" << day
        << hour << ":" << minute << ":" << second;
#else
    Maemo::Timed::WallClock::Settings s;
    QDateTime newTime(QDate(year, month, day), QTime(hour, minute, second));
    newTime.setTimeZone(QTimeZone::systemTimeZone());
    s.setTimeManual(newTime.toTime_t());

    Maemo::Timed::Interface timed;
    timed.wall_clock_settings_async(s);
#endif
}

QLowEnergyServiceData TimeService::createTimeServiceData() {
    QLowEnergyCharacteristicData timeUpdateData;
    timeUpdateData.setUuid(TimeUpdateUuid);
    timeUpdateData.setProperties(QLowEnergyCharacteristic::WriteNoResponse);
    timeUpdateData.setValue(QByteArray()); // empty value initially
#ifndef QT_IS_BUGGY
    // this should not be necessary, but without it, the characteristic is never marked valid
    // on the receiving side.
    QLowEnergyDescriptorData cccd(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
    timeUpdateData.addDescriptor(cccd);
#endif
    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(TimeServiceUuid);
    serviceData.addCharacteristic(timeUpdateData);

    return serviceData;
}

