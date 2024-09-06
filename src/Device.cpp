#include "Device.h"

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
    // TODO: add code to read system name
    m_bluetoothService.startAdvertising("catfish3");
}

