#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include "BluetoothService.h"
#include "BatteryService.h"
#include "HeartRateService.h"
#include "NotificationService.h"
#include "TimeService.h"
#include "MediaService.h"
#include "ScreenshotService.h"

class Device : public QObject {
    Q_OBJECT

public:
    explicit Device(QObject *parent = nullptr);

private:
    BluetoothService m_bluetoothService;
    BatteryService m_batteryService;
    HeartRateService m_heartRateService;
    NotificationService m_notificationService;
    TimeService m_timeService;
    MediaService m_mediaService;
    ScreenshotService m_screenshotService;
};

#endif // DEVICE_H

