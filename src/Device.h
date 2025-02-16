#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtBluetooth/QLowEnergyController>

#include "BluetoothService.h"
#include "Remote.h"

#include "BatteryService.h"
#include "HeartRateService.h"
#include "NotificationService.h"
#include "TimeService.h"
#include "MediaService.h"
#include "ScreenshotService.h"
#include "WeatherService.h"


class Device : public QObject {
    Q_OBJECT

public:
    explicit Device(QObject *parent = nullptr);
private slots:
    void onDeviceConnected(QBluetoothAddress remote, QBluetoothAddress local);
    void onDeviceDisconnected();
    void onServiceDiscovered(const QBluetoothUuid &newService);
private:
    // the BluetoothService drives everything else
    BluetoothService *m_bluetoothService;

    // these are the basic included services
    BatteryService *m_batteryService;
    HeartRateService *m_heartRateService;
    NotificationService *m_notificationService;
    TimeService *m_timeService;
    MediaService *m_mediaService;
    ScreenshotService *m_screenshotService;
    WeatherService *m_weatherService;

    // this is for the remote (paired) device for reverse services
    // Note that it is ephemeral and only exists for the duration 
    // of the connection.
    Remote *m_remote = nullptr;
};

#endif // DEVICE_H

