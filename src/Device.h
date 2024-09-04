#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include "BluetoothService.h"
#include "BatteryService.h"
#include "HeartRateService.h"
#include "NotificationService.h"

class Device : public QObject {
    Q_OBJECT

public:
    explicit Device(QObject *parent = nullptr);

private:
    BluetoothService m_bluetoothService;
    BatteryService m_batteryService;
    HeartRateService m_heartRateService;
    NotificationService m_notificationService;
};

#endif // DEVICE_H

