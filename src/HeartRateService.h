#ifndef HEARTRATESERVICE_H
#define HEARTRATESERVICE_H

#include "asteroid-btsyncd.h"
#include <QObject>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyServiceData>
#ifndef DESKTOP_VERSION
#include <QtSensors/QHrmSensor>
#else
class QHrmSensor : public QObject {
    Q_OBJECT
public:
    QHrmSensor(QObject *parent) {};
    struct Reading {
        int bpm() { return 62; }
    } rdg;
    void start();
    Reading *reading() { return &rdg; }
    enum Status { NoContact, Unreliable, AccuracyLow, AccuracyMedium, AccuracyHigh};
signals:
    void statusChanged(QHrmSensor::Status status);
};
#endif
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

