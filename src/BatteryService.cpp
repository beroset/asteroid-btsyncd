#include "BatteryService.h"
#ifndef DESKTOP_VERSION
#include <batterystatus.h>
#endif
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyDescriptorData>

BatteryService::BatteryService(BluetoothService &bluetoothService, QObject *parent) : QObject(parent) {
    m_battery = new BatteryStatus(this);
    QLowEnergyServiceData serviceData = createBatteryServiceData();
    connect(m_battery, &BatteryStatus::chargePercentageChanged,
            this, &BatteryService::onBatteryPercentageChanged);
    m_service = bluetoothService.addService(serviceData);
}

void BatteryService::onBatteryPercentageChanged(int percentage)
{
    QLowEnergyCharacteristic characteristic = m_service->characteristic(QBluetoothUuid::BatteryLevel);
    Q_ASSERT(characteristic.isValid());
    m_service->writeCharacteristic(characteristic, QByteArray(1, percentage));
}

QLowEnergyServiceData BatteryService::createBatteryServiceData() {
    QLowEnergyCharacteristicData batteryLevelData;
    batteryLevelData.setUuid(QBluetoothUuid::BatteryLevel);
    batteryLevelData.setValue(QByteArray(1, 100));
    batteryLevelData.setProperties(QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::Notify);

    QLowEnergyDescriptorData cccd(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
    batteryLevelData.addDescriptor(cccd);

    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(QBluetoothUuid::BatteryService);
    serviceData.addCharacteristic(batteryLevelData);

    return serviceData;
}

