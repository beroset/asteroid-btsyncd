#ifndef ANCSSERVICE_H
#define ANCSSERVICE_H

#include <QObject>
#include <QtBluetooth/QLowEnergyService>
#include <QtBluetooth/QLowEnergyController>
#include <QtBluetooth/QLowEnergyServiceData>
#include "BluetoothService.h"

class ANCSService : public QObject {
    Q_OBJECT

public:
    explicit ANCSService(BluetoothService &bluetoothService, QObject *parent = nullptr);
    void connectToDevice(const QBluetoothDeviceInfo &deviceInfo);

signals:
    void notificationReceived(const QByteArray &data);
    void dataSourceReceived(const QByteArray &data);

private slots:
    void onServiceDiscovered(const QBluetoothUuid &uuid);
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

private:
    QLowEnergyController *m_controller;
    QLowEnergyService *m_ancsService;
    QLowEnergyCharacteristic m_notificationSourceChar;
    QLowEnergyCharacteristic m_controlPointChar;
    QLowEnergyCharacteristic m_dataSourceChar;

    void discoverService();
    void subscribeToNotifications();
    void processNotificationSource(const QByteArray &data);
    void processDataSource(const QByteArray &data);
};

#endif // ANCSSERVICE_H

