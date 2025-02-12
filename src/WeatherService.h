#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include "asteroid-btsyncd.h"
#include <QObject>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyServiceData>
#include "BluetoothService.h"

class WeatherService : public QObject {
    Q_OBJECT

public:
    explicit WeatherService(BluetoothService &bluetoothService, QObject *parent = nullptr);
    void add(BluetoothService &bluetoothService);
    void remove();
    QLowEnergyService* service() const;

private slots:
    void onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

private:
    QLowEnergyServiceData createWeatherServiceData();
    QLowEnergyService *m_service;
};

#endif // WEATHERSERVICE_H

