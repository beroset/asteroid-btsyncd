#ifndef BLUETOOTHSERVICE_H
#define BLUETOOTHSERVICE_H

#include <QObject>
#include <QtBluetooth/QLowEnergyController>
#include <QtBluetooth/QLowEnergyService>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyDescriptor>

class BluetoothService : public QObject {
    Q_OBJECT

public:
    explicit BluetoothService(QObject *parent = nullptr);
    void addService(const QLowEnergyServiceData &serviceData);
    void startAdvertising(const QString &localName);

private slots:
    void onControllerStateChanged(QLowEnergyController::ControllerState state);
    void onErrorOccurred(QLowEnergyController::Error error);

private:
    QLowEnergyController *m_controller = nullptr;
    QList<QLowEnergyService *> m_services;
};

#endif // BLUETOOTHSERVICE_H

