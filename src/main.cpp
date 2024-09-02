/*
 * Copyright (C) 2016 - Florent Revest <revestflo@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtBluetooth/qlowenergyadvertisingdata.h>
#include <QtBluetooth/qlowenergyadvertisingparameters.h>
#include <QtBluetooth/qlowenergycharacteristic.h>
#include <QtBluetooth/qlowenergycharacteristicdata.h>
#include <QtBluetooth/qlowenergydescriptordata.h>
#include <QtBluetooth/qlowenergycontroller.h>
#include <QtBluetooth/qlowenergyservice.h>
#include <QtBluetooth/qlowenergyservicedata.h>

#include <QBluetoothUuid>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristicData>

#include <QtCore/qbytearray.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qlist.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qtimer.h>
#include <QTranslator>

class BtService {
public:
    BtService();
    const QLowEnergyServiceData& service() const;
    void run(QLowEnergyService& service);
private:
    QLowEnergyCharacteristicData charData;
    QLowEnergyServiceData serviceData;
};

BtService::BtService() 
{
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

int main(int argc, char *argv[])
{
    //QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));
#ifndef Q_OS_ANDROID
    QCoreApplication app(argc, argv);
#else
    QGuiApplication app(argc, argv);
#endif

    //! [Advertising Data]
    QLowEnergyAdvertisingData advertisingData;
    advertisingData.setDiscoverability(QLowEnergyAdvertisingData::DiscoverabilityGeneral);
    advertisingData.setIncludePowerLevel(true);
    advertisingData.setLocalName("catfish3");
    advertisingData.setServices(QList<QBluetoothUuid>() << QBluetoothUuid::HeartRate);
    //! [Advertising Data]

    //! [Service Data]
    BtService svc;
    //! [Service Data]

    //! [Start Advertising]
    const QScopedPointer<QLowEnergyController> leController(QLowEnergyController::createPeripheral());
    QScopedPointer<QLowEnergyService> service(leController->addService(svc.service()));
    leController->startAdvertising(QLowEnergyAdvertisingParameters(), advertisingData,
                                   advertisingData);
    //! [Start Advertising]

    //! [Provide Heartbeat]
    svc.run(*service);
    //! [Provide Heartbeat]

    auto reconnect = [&leController, advertisingData, &service, &svc]()
    {
        service.reset(leController->addService(svc.service()));
        if (!service.isNull())
            leController->startAdvertising(QLowEnergyAdvertisingParameters(),
                                           advertisingData, advertisingData);
    };
    QObject::connect(leController.data(), &QLowEnergyController::disconnected, reconnect);

    return app.exec();
}
