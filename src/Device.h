#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtBluetooth/QLowEnergyController>

#include "BluetoothService.h"
#include "BatteryService.h"
#include "HeartRateService.h"
#include "NotificationService.h"
#include "TimeService.h"
#include "MediaService.h"
#include "ScreenshotService.h"
#include "WeatherService.h"
#include "ANCSService.h"


class Device : public QObject {
    Q_OBJECT

public:
    explicit Device(QObject *parent = nullptr);
#if 0
private slots:
    void onDeviceConnected();
    void onNotificationReceived(const QByteArray &data);
    void onDataSourceReceived(const QByteArray &data);
#endif
private:
    BluetoothService m_bluetoothService;
    BatteryService m_batteryService;
    HeartRateService m_heartRateService;
    NotificationService m_notificationService;
    TimeService m_timeService;
    MediaService m_mediaService;
    ScreenshotService m_screenshotService;
    WeatherService m_weatherService;
    ANCSService m_ancsService;

    QLowEnergyController *m_controller;
    
    //void setupController();
};

#endif // DEVICE_H

