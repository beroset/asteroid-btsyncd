#include "HeartRateService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyDescriptorData>

HeartRateService::HeartRateService(BluetoothService &bluetoothService, QObject *parent) : QObject(parent) {
    QLowEnergyServiceData serviceData = createHeartRateServiceData();
    bluetoothService.addService(serviceData);
}

QLowEnergyServiceData HeartRateService::createHeartRateServiceData() {
    QLowEnergyCharacteristicData heartRateMeasurementData;
    heartRateMeasurementData.setUuid(QBluetoothUuid::HeartRateMeasurement);
    heartRateMeasurementData.setValue(QByteArray(1, 60)); // 60 bpm heart rate
    heartRateMeasurementData.setProperties(QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::Notify);

    QLowEnergyDescriptorData cccd(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
    heartRateMeasurementData.addDescriptor(cccd);

    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(QBluetoothUuid::HeartRate);
    serviceData.addCharacteristic(heartRateMeasurementData);

    return serviceData;
}

