#include "NotificationService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyDescriptorData>

static const QBluetoothUuid NotificationServiceUuid{QString{"00009071-0000-0000-0000-00A57E401D05"}};
static const QBluetoothUuid NotificationUpdateUuid{QString{"00009001-0000-0000-0000-00A57E401D05"}};

NotificationService::NotificationService(BluetoothService &bluetoothService, QObject *parent) : QObject(parent) {
    QLowEnergyServiceData serviceData = createNotificationServiceData();
    m_service = bluetoothService.addService(serviceData);
}

void NotificationService::onNotificationClosed(uint replacesId, uint)
{
#if 0
    QLowEnergyCharacteristic characteristic = m_service->characteristic(QBluetoothUuid::NotificationLevel);
    Q_ASSERT(characteristic.isValid());
    m_service->writeCharacteristic(characteristic, QByteArray(1, percentage));
#endif
}

QLowEnergyServiceData NotificationService::createNotificationServiceData() {
    QLowEnergyCharacteristicData notificationUpdateData;
    notificationUpdateData.setUuid(NotificationUpdateUuid);
    notificationUpdateData.setProperties(QLowEnergyCharacteristic::Write);

    QLowEnergyDescriptorData cccd(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
    notificationUpdateData.addDescriptor(cccd);

    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(NotificationServiceUuid);
    serviceData.addCharacteristic(notificationUpdateData);

    return serviceData;
}

