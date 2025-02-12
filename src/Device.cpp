#include "Device.h"
#include <unistd.h>

static constexpr unsigned namesize{10};
static char hostname[namesize];

Device::Device(QObject *parent)
    : QObject(parent)
{
    m_bluetoothService = new BluetoothService(this);
#if 0
    m_batteryService = new BatteryService(m_bluetoothService, this);
    m_heartRateService = new HeartRateService(m_bluetoothService, this);
    m_notificationService = new NotificationService(m_bluetoothService, this);
    m_timeService = new TimeService(m_bluetoothService, this);
    m_mediaService = new MediaService(m_bluetoothService, this);
    m_screenshotService = new ScreenshotService(m_bluetoothService, this);
    m_weatherService = new WeatherService(m_bluetoothService, this);
#endif
    if (gethostname(hostname, namesize) != 0) {
        qCDebug(btsyncd) << "Unable to read hostname";
    }
    m_bluetoothService->startAdvertising(hostname);
    connect(m_bluetoothService, &BluetoothService::deviceConnected, this, &Device::onDeviceConnected);
    connect(m_bluetoothService, &BluetoothService::deviceDisconnected, this, &Device::onDeviceDisconnected);
}

void Device::onDeviceDisconnected()
{
    if (m_remote) {
        qDebug() << "Disconnecting from " << m_remote->remoteAddress();
        m_remote->deleteLater();  // delete the existing one if it exists
    }
    disconnect(m_bluetoothService, nullptr, this, nullptr);
    m_bluetoothService->deleteLater();
    m_bluetoothService = new BluetoothService(this);

#if 0
    delete m_batteryService;
    delete m_heartRateService;
    delete m_notificationService;
    delete m_timeService;
    delete m_mediaService;
    delete m_screenshotService;
    delete m_weatherService;
    m_batteryService = new BatteryService(m_bluetoothService, this);
    m_heartRateService = new HeartRateService(m_bluetoothService, this);
    m_notificationService = new NotificationService(m_bluetoothService, this);
    m_timeService = new TimeService(m_bluetoothService, this);
    m_mediaService = new MediaService(m_bluetoothService, this);
    m_screenshotService = new ScreenshotService(m_bluetoothService, this);
    m_weatherService = new WeatherService(m_bluetoothService, this);
#endif
    // now reconnect
    connect(m_bluetoothService, &BluetoothService::deviceConnected, this, &Device::onDeviceConnected);
    connect(m_bluetoothService, &BluetoothService::deviceDisconnected, this, &Device::onDeviceDisconnected);

    m_bluetoothService->startAdvertising(hostname);
}

void Device::onDeviceConnected(QBluetoothAddress remote, QBluetoothAddress local)
{
    if (m_remote) {
        m_remote->deleteLater();  // delete the existing one if it exists
        m_remote = new Remote(remote, local, this);
    }
    qDebug() << "Connecting with " << remote << " from " << local;
}
