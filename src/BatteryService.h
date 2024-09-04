#ifndef BATTERYSERVICE_H
#define BATTERYSERVICE_H

#include <QObject>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyServiceData>
#include "BluetoothService.h"

class BatteryService : public QObject {
    Q_OBJECT

public:
    explicit BatteryService(BluetoothService &bluetoothService, QObject *parent = nullptr);

private:
    QLowEnergyServiceData createBatteryServiceData();
};

#endif // BATTERYSERVICE_H

