#ifndef BLUETOOTHSERVICE_H
#define BLUETOOTHSERVICE_H

#include "asteroid-btsyncd.h"
#include <QObject>
#include <QtBluetooth/QLowEnergyController>
#include <QtBluetooth/QLowEnergyService>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyDescriptor>
#include <QBluetoothLocalDevice>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(btsyncd)

class BluetoothService : public QObject {
    Q_OBJECT

public:
    explicit BluetoothService(QObject *parent = nullptr);
    QLowEnergyService* addService(const QLowEnergyServiceData &serviceData);
    void startAdvertising(const QString &localName);

signals:
    void deviceConnected(QBluetoothAddress remote, QBluetoothAddress local);
    void deviceDisconnected();

private slots:
    void onControllerStateChanged(QLowEnergyController::ControllerState state);
    void onErrorOccurred(QLowEnergyController::Error error);
    void onPairingFinished(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing pairing);

private:
    void pairDevice(const QBluetoothAddress &address);
    void reset();
    QLowEnergyController *m_controller = nullptr;
    QBluetoothLocalDevice *m_local = nullptr;
    QList<QLowEnergyService *> m_services;
};

#endif // BLUETOOTHSERVICE_H

