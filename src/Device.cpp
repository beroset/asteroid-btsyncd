#include "Device.h"
#include <unistd.h>

Device::Device(QObject *parent)
    : QObject(parent)
    , m_batteryService(m_bluetoothService, this)
    , m_heartRateService(m_bluetoothService, this)
    , m_notificationService(m_bluetoothService, this)
    , m_timeService(m_bluetoothService, this)
    , m_mediaService(m_bluetoothService, this)
    , m_screenshotService(m_bluetoothService, this)
    , m_weatherService(m_bluetoothService, this)
{
    static constexpr unsigned namesize{10};
    char hostname[namesize];
    if (gethostname(hostname, namesize) != 0) {
        qCDebug(btsyncd) << "Unable to read hostname";
    }
    m_bluetoothService.startAdvertising(hostname);
    connect(&m_bluetoothService, &BluetoothService::deviceConnected, this, &Device::onDeviceConnected);
    connect(&m_bluetoothService, &BluetoothService::deviceDisconnected, this, &Device::onDeviceDisconnected);
}

void Device::onDeviceDisconnected()
{
    delete m_remote;  // delete the existing one if it exists    
    m_remote = nullptr;
}

void Device::onDeviceConnected(QBluetoothAddress remote, QBluetoothAddress local)
{
    delete m_remote;  // delete the existing one if it exists    
    m_remote = new Remote(remote, local, this);
}
