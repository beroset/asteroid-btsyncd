#ifndef HEARTRATESERVICE_H
#define HEARTRATESERVICE_H

#include <QObject>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyServiceData>
#include <QtSensors/QHrmSensor>
#include "BluetoothService.h"

class HeartRateService : public QObject {
    Q_OBJECT

public:
    explicit HeartRateService(BluetoothService &bluetoothService, QObject *parent = nullptr);
    QLowEnergyService* service() const;

private slots:
    void onStatusChanged(QHrmSensor::Status status);

private:
    QLowEnergyServiceData createHeartRateServiceData();
    QLowEnergyService *m_service;
    QHrmSensor *m_hrm;
};

#endif // HEARTRATESERVICE_H

