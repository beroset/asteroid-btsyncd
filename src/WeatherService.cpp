#include "asteroid-btsyncd.h"
#ifndef DESKTOP_VERSION
#include <giomm.h>
#endif

#include "WeatherService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyDescriptorData>
#include <QDebug>

static const QBluetoothUuid WeatherServiceUuid{QStringLiteral("00008071-0000-0000-0000-00A57E401D05")};
static const QBluetoothUuid WeatherCityUuid{QStringLiteral("00008001-0000-0000-0000-00A57E401D05")};
static const QBluetoothUuid WeatherIdsUuid{QStringLiteral("00008002-0000-0000-0000-00A57E401D05")};
static const QBluetoothUuid WeatherMinTUuid{QStringLiteral("00008003-0000-0000-0000-00A57E401D05")};
static const QBluetoothUuid WeatherMaxTUuid{QStringLiteral("00008004-0000-0000-0000-00A57E401D05")};

static int getQByteArrayInt(QByteArray arr, int index) {
    return (((unsigned char) arr[index * 2]) << 8) | ((unsigned char) arr[index * 2 + 1]);
}

WeatherService::WeatherService(BluetoothService &bluetoothService, QObject *parent) : QObject(parent) {
    add(bluetoothService);
}

void WeatherService::add(BluetoothService &bluetoothService)
{
    QLowEnergyServiceData serviceData = createWeatherServiceData();
    m_service = bluetoothService.addService(serviceData);
    connect(m_service, &QLowEnergyService::characteristicChanged, this, &WeatherService::onCharacteristicWritten);
}

void WeatherService::remove()
{
    disconnect(m_service, nullptr, this, nullptr);
}

QLowEnergyService* WeatherService::service() const {
    return m_service;
}

void WeatherService::onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value) 
{
#ifdef DESKTOP_VERSION
    if (characteristic.uuid() == WeatherCityUuid) {
        qDebug() << "city-name = " << value.data();
    } else if (characteristic.uuid() == WeatherIdsUuid) {
        for(int i = 0; i < 5; i++) {
            qDebug() << "day[" << i << "].id = " << value;
        }
    } else if (characteristic.uuid() == WeatherMinTUuid) {
        for(int i = 0; i < 5; i++) {
            qDebug() << "day[" << i << "].min-temp = " << value;
        }
    } else if (characteristic.uuid() == WeatherMaxTUuid) {
        for(int i = 0; i < 5; i++) {
            qDebug() << "day[" << i << "].max-temp = " << value;
        }
    }
#else
    if (characteristic.uuid() == WeatherCityUuid) {
        const Glib::RefPtr<Gio::Settings> settings = Gio::Settings::create("org.asteroidos.weather");
        settings->set_string("city-name", value.data());
    } else if (characteristic.uuid() == WeatherIdsUuid) {
        for(int i = 0; i < 5; i++) {
            const Glib::RefPtr<Gio::Settings> settings = Gio::Settings::create("org.asteroidos.weather.day" + std::to_string(i));
            settings->set_int("id", getQByteArrayInt(value, i));
        }

        const Glib::RefPtr<Gio::Settings> settings = Gio::Settings::create("org.asteroidos.weather");
        settings->set_int("timestamp-day0", (int)time(NULL));
    } else if (characteristic.uuid() == WeatherMinTUuid) {
        for(int i = 0; i < 5; i++) {
            const Glib::RefPtr<Gio::Settings> settings = Gio::Settings::create("org.asteroidos.weather.day" + std::to_string(i));
            settings->set_int("min-temp", getQByteArrayInt(value, i));
        }
    } else if (characteristic.uuid() == WeatherMaxTUuid) {
        for(int i = 0; i < 5; i++) {
            const Glib::RefPtr<Gio::Settings> settings = Gio::Settings::create("org.asteroidos.weather.day" + std::to_string(i));
            settings->set_int("max-temp", getQByteArrayInt(value, i));
        }
    }
#endif
}

QLowEnergyServiceData WeatherService::createWeatherServiceData() {
    QLowEnergyCharacteristicData weatherCityData;
    weatherCityData.setUuid(WeatherCityUuid);
    weatherCityData.setProperties(QLowEnergyCharacteristic::Write);
    weatherCityData.setValue(QByteArray()); // empty value initially

    QLowEnergyCharacteristicData weatherIdsData;
    weatherIdsData.setUuid(WeatherIdsUuid);
    weatherIdsData.setProperties(QLowEnergyCharacteristic::Write);
    weatherIdsData.setValue(QByteArray()); // empty value initially

    QLowEnergyCharacteristicData weatherMinTData;
    weatherMinTData.setUuid(WeatherMinTUuid);
    weatherMinTData.setProperties(QLowEnergyCharacteristic::Write);
    weatherMinTData.setValue(QByteArray()); // empty value initially

    QLowEnergyCharacteristicData weatherMaxTData;
    weatherMaxTData.setUuid(WeatherMaxTUuid);
    weatherMaxTData.setProperties(QLowEnergyCharacteristic::Write);
    weatherMaxTData.setValue(QByteArray()); // empty value initially
#ifndef QT_IS_BUGGY
    // this should not be necessary, but without it, the characteristic s never marked valid 
    // on the receiving side.
    QLowEnergyDescriptorData cccd(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
    weatherCityData.addDescriptor(cccd);
#endif
    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(WeatherServiceUuid);
    serviceData.addCharacteristic(weatherCityData);
    serviceData.addCharacteristic(weatherIdsData);
    serviceData.addCharacteristic(weatherMinTData);
    serviceData.addCharacteristic(weatherMaxTData);

    return serviceData;
}

