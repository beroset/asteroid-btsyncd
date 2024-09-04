#ifndef BATTERYSERVICE_H
#define BATTERYSERVICE_H

#include <QObject>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyServiceData>
#include "BluetoothService.h"

class BatteryStatus;

class BatteryService : public QObject {
    Q_OBJECT

public:
    explicit BatteryService(BluetoothService &bluetoothService, QObject *parent = nullptr);
    QLowEnergyService* service() const;

private slots:
    void onBatteryPercentageChanged(int percentage);

private:
    QLowEnergyServiceData createBatteryServiceData();
    QLowEnergyService *m_service;
    BatteryStatus *m_battery;
};

#endif // BATTERYSERVICE_H

