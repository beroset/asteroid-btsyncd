#include "BatteryService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyDescriptorData>

BatteryService::BatteryService(BluetoothService &bluetoothService, QObject *parent) : QObject(parent) {
    QLowEnergyServiceData serviceData = createBatteryServiceData();
    bluetoothService.addService(serviceData);
}

QLowEnergyServiceData BatteryService::createBatteryServiceData() {
    QLowEnergyCharacteristicData batteryLevelData;
    batteryLevelData.setUuid(QBluetoothUuid::BatteryLevel);
    batteryLevelData.setValue(QByteArray(1, 50)); // 50% battery level
    batteryLevelData.setProperties(QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::Notify);

    QLowEnergyDescriptorData cccd(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
    batteryLevelData.addDescriptor(cccd);

    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(QBluetoothUuid::BatteryService);
    serviceData.addCharacteristic(batteryLevelData);

    return serviceData;
}

