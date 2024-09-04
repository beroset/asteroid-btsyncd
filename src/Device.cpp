#include "Device.h"

Device::Device(QObject *parent)
    : QObject(parent)
    , m_batteryService(m_bluetoothService, this)
    , m_heartRateService(m_bluetoothService, this)
    , m_notificationService(m_bluetoothService, this)
{
    m_bluetoothService.startAdvertising("catfish3");
}

