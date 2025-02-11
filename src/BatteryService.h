#ifndef BATTERYSERVICE_H
#define BATTERYSERVICE_H

#include "asteroid-btsyncd.h"
#include <QObject>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyServiceData>
#include "BluetoothService.h"
#ifdef DESKTOP_VERSION
class BatteryStatus : public QObject {
    Q_OBJECT
public:
    BatteryStatus(QObject *parent) {};
signals:
    void chargePercentageChanged(int percentage);
};
#else
class BatteryStatus;
#endif

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

