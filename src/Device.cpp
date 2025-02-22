#include "Device.h"
#include <unistd.h>

static constexpr unsigned namesize{10};
static char hostname[namesize];

Device::Device(QObject *parent)
    : QObject(parent)
{
    m_bluetoothService = new BluetoothService(this);
    m_batteryService = new BatteryService(*m_bluetoothService, this);
    m_heartRateService = new HeartRateService(*m_bluetoothService, this);
    m_notificationService = new NotificationService(*m_bluetoothService, this);
    m_timeService = new TimeService(*m_bluetoothService, this);
    m_mediaService = new MediaService(*m_bluetoothService, this);
    m_screenshotService = new ScreenshotService(*m_bluetoothService, this);
    m_weatherService = new WeatherService(*m_bluetoothService, this);
    if (gethostname(hostname, namesize) != 0) {
        qCDebug(btsyncd) << "Unable to read hostname";
    }
    m_bluetoothService->startAdvertising(hostname);
    connect(m_bluetoothService, &BluetoothService::deviceConnected, this, &Device::onDeviceConnected);
    connect(m_bluetoothService, &BluetoothService::deviceDisconnected, this, &Device::onDeviceDisconnected);
}

void Device::onDeviceDisconnected()
{
    qDebug() << "Disconnecting from " << m_remote->remoteAddress();

    // unplug all of the services
    m_batteryService->remove();
    m_heartRateService->remove();
    m_notificationService->remove();
    m_timeService->remove();
    m_mediaService->remove();
    m_screenshotService->remove();
    m_weatherService->remove();

    disconnect(m_bluetoothService, nullptr, this, nullptr);
    m_bluetoothService->deleteLater();
    m_remote->deleteLater();
    m_bluetoothService = new BluetoothService(this);

    m_batteryService->add(*m_bluetoothService);
    m_heartRateService->add(*m_bluetoothService);
    m_notificationService->add(*m_bluetoothService);
    m_timeService->add(*m_bluetoothService);
    m_mediaService->add(*m_bluetoothService);
    m_screenshotService->add(*m_bluetoothService);
    m_weatherService->add(*m_bluetoothService);

    // now reconnect
    connect(m_bluetoothService, &BluetoothService::deviceConnected, this, &Device::onDeviceConnected);
    connect(m_bluetoothService, &BluetoothService::deviceDisconnected, this, &Device::onDeviceDisconnected);

    m_bluetoothService->startAdvertising(hostname);
}

static const QBluetoothUuid ANCS_SERVICE_UUID{QStringLiteral("7905F431-B5CE-4E99-A40F-4B1E122D00D0")};

void Device::onDeviceConnected(QBluetoothAddress remote, QBluetoothAddress local)
{
    qDebug() << "Connecting with " << remote << " from " << local;
    m_remote = new Remote(remote, local, this);
    connect(m_remote, &Remote::serviceDiscovered, this, &Device::onServiceDiscovered);
    m_remote->discoverServices();
}


void Device::onServiceDiscovered(const QBluetoothUuid &newService)
{
    if (newService == ANCS_SERVICE_UUID) {
        qDebug() << "ANCS discovered";
    } else if (newService == QBluetoothUuid::BatteryService) {
        qDebug() << "Battery service discovered";
    } else {
        qDebug() << "Other service discovered: " << newService;
    }
}
