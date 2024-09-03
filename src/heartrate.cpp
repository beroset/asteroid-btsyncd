#include "heartrate.h"
#include <QLowEnergyCharacteristicData>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyDescriptorData>
#include <QBluetoothUuid>
#include <QTimer>

BtService::BtService() 
{
    QLowEnergyCharacteristicData charData;
    charData.setUuid(QBluetoothUuid::HeartRateMeasurement);
    charData.setValue(QByteArray(2, 0));
    charData.setProperties(QLowEnergyCharacteristic::Notify | QLowEnergyCharacteristic::Read);
    const QLowEnergyDescriptorData clientConfig(QBluetoothUuid::ClientCharacteristicConfiguration,
                                                QByteArray(2, 0));
    charData.addDescriptor(clientConfig);

    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(QBluetoothUuid::HeartRate);
    serviceData.addCharacteristic(charData);
}

const QLowEnergyServiceData& BtService::service() const
{
    return serviceData;
}

void BtService::run(QLowEnergyService& service)
{
    QTimer heartbeatTimer;
    quint8 currentHeartRate = 60;
    enum ValueChange { ValueUp, ValueDown } valueChange = ValueUp;
    const auto heartbeatProvider = [&service, &currentHeartRate, &valueChange]() {
        QByteArray value;
        value.append(char(0));
        value.append(char(currentHeartRate)); // Actual value.
        QLowEnergyCharacteristic characteristic
                = service.characteristic(QBluetoothUuid::HeartRateMeasurement);
        Q_ASSERT(characteristic.isValid());
        service.writeCharacteristic(characteristic, value); // Potentially causes notification.
        if (currentHeartRate == 60)
            valueChange = ValueUp;
        else if (currentHeartRate == 100)
            valueChange = ValueDown;
        if (valueChange == ValueUp)
            ++currentHeartRate;
        else
            --currentHeartRate;
    };
    QObject::connect(&heartbeatTimer, &QTimer::timeout, heartbeatProvider);
    heartbeatTimer.start(1000);
}
