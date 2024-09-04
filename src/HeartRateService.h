#ifndef HEARTRATESERVICE_H
#define HEARTRATESERVICE_H

#include <QObject>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyServiceData>
#include "BluetoothService.h"

class HeartRateService : public QObject {
    Q_OBJECT

public:
    explicit HeartRateService(BluetoothService &bluetoothService, QObject *parent = nullptr);
    QLowEnergyService* service() const;

private:
    QLowEnergyServiceData createHeartRateServiceData();
    QLowEnergyService *m_service;
};

#endif // HEARTRATESERVICE_H

