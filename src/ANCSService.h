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
    QLowEnergyService* service() const;

#if 0
signals:
    void notificationReceived(const QByteArray &data);
    void dataSourceReceived(const QByteArray &data);
#endif 

private slots:
    void checkForANCS(QBluetoothAddress remote, QBluetoothAddress local);
#if 0
    void onServiceDiscovered(const QBluetoothUuid &uuid);
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
#endif

private:
    QLowEnergyService *m_service = nullptr;
    QLowEnergyController *controller = nullptr;
#if 0
    QLowEnergyService *m_ancsService;
    QLowEnergyCharacteristic m_notificationSourceChar;
    QLowEnergyCharacteristic m_controlPointChar;
    QLowEnergyCharacteristic m_dataSourceChar;

    void discoverService();
    void subscribeToNotifications();
    void processNotificationSource(const QByteArray &data);
    void processDataSource(const QByteArray &data);
#endif 
};
#endif // ANCSSERVICE_H

