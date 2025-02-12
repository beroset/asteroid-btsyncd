#ifndef NOTIFICATIONSERVICE_H
#define NOTIFICATIONSERVICE_H

#include "asteroid-btsyncd.h"
#include <QObject>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyServiceData>
#include <QHash>
#include "BluetoothService.h"

class NotificationService : public QObject {
    Q_OBJECT

public:
    explicit NotificationService(BluetoothService &bluetoothService, QObject *parent = nullptr);
    void add(BluetoothService &bluetoothService);
    void remove();
    QLowEnergyService* service() const;

private slots:
    void onNotificationClosed(uint replacesId, uint);
    void onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

private:
    QLowEnergyServiceData createNotificationServiceData();
    QLowEnergyService *m_service;
    QHash<int, uint> mKnownAndroidNotifs;
};

#endif // NOTIFICATIONSERVICE_H

