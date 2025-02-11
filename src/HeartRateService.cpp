#include "HeartRateService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyDescriptorData>

HeartRateService::HeartRateService(BluetoothService &bluetoothService, QObject *parent) : QObject(parent) {
    m_hrm = new QHrmSensor(this);
    m_hrm->start();
    QLowEnergyServiceData serviceData = createHeartRateServiceData();
    connect(m_hrm, &QHrmSensor::statusChanged,
        this, &HeartRateService::onStatusChanged);
    m_service = bluetoothService.addService(serviceData);
}

void HeartRateService::onStatusChanged(QHrmSensor::Status status) {
    static QStringList labels = { "NoContact", "Unreliable", "AccuracyLow", "AccuracyMedium", "AccuracyHigh"};
    auto value = m_hrm->reading()->bpm();
    qDebug() << "HRM status is now " << labels[status] << ", value = " << value << " bpm";
    QLowEnergyCharacteristic characteristic = m_service->characteristic(QBluetoothUuid::HeartRateMeasurement);
    Q_ASSERT(characteristic.isValid());
    QByteArray bytes{};
    char constexpr skin_contact_feature{0x4};
    char constexpr skin_contact_detected{0x6};
    char flag{status ? skin_contact_detected : skin_contact_feature};
    bytes.append(flag);  // flag value
    bytes.append(value);
    m_service->writeCharacteristic(characteristic, bytes);
}

QLowEnergyServiceData HeartRateService::createHeartRateServiceData() {
    QLowEnergyCharacteristicData heartRateMeasurementData;
    heartRateMeasurementData.setUuid(QBluetoothUuid::HeartRateMeasurement);
    QByteArray value{};
    char constexpr skin_contact_feature{0x4};
    char flag{skin_contact_feature};
    value.append(flag);
    value.append(60);  // 60 bpm
    heartRateMeasurementData.setValue(value); // 0 bpm heart rate
    heartRateMeasurementData.setProperties(QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::Notify);

    QLowEnergyDescriptorData cccd(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
    heartRateMeasurementData.addDescriptor(cccd);

    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(QBluetoothUuid::HeartRate);
    serviceData.addCharacteristic(heartRateMeasurementData);

    return serviceData;
}

