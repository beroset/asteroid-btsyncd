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
    , m_ancsService(m_bluetoothService, this)
{
    // TODO: add code to read system name
    m_bluetoothService.startAdvertising("catfish3");
#if 0
    // In this example, we assume the iPhone initiates the connection.
    setupController();
}

void Device::setupController() {
    // This would normally be set based on an event indicating an incoming connection.
    // For the sake of this example, assume a QBluetoothDeviceInfo instance is available.
    // QBluetoothDeviceInfo deviceInfo = ...;

    // In practice, you would obtain the deviceInfo from a connection event.
    // m_controller = QLowEnergyController::createCentral(deviceInfo, this);

    // For demonstration purposes, assuming controller is set up:
    if (m_controller) {
//        m_ancsService.setController(m_controller);
    }

    connect(&m_ancsService, &ANCSService::notificationReceived, this, &Device::onNotificationReceived);
    connect(&m_ancsService, &ANCSService::dataSourceReceived, this, &Device::onDataSourceReceived);
}

void Device::onNotificationReceived(const QByteArray &data) {
    qDebug() << "Notification received:" << data;
    // Handle notification source data here
}

void Device::onDataSourceReceived(const QByteArray &data) {
    qDebug() << "Data source received:" << data;
    // Handle data source data here
#endif
}
