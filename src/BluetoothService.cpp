#include "BluetoothService.h"
#include <QtBluetooth/QLowEnergyAdvertisingData>
#include <QtBluetooth/QLowEnergyAdvertisingParameters>
#include <QDebug>

static const QBluetoothUuid AsteroidOSUuid{QString{"00000000-0000-0000-0000-00a57e401d05"}};

BluetoothService::BluetoothService(QObject *parent) : QObject(parent) {
    m_controller = QLowEnergyController::createPeripheral(this);

    connect(m_controller, &QLowEnergyController::stateChanged, this, &BluetoothService::onControllerStateChanged);
    connect(m_controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this, &BluetoothService::onErrorOccurred);
}

QLowEnergyService* BluetoothService::addService(const QLowEnergyServiceData &serviceData) {
    QLowEnergyService *service = m_controller->addService(serviceData);
    m_services.append(service);
    return service;
}

void BluetoothService::startAdvertising(const QString &localName) {
    QLowEnergyAdvertisingData advertisingData;
    advertisingData.setDiscoverability(QLowEnergyAdvertisingData::DiscoverabilityGeneral);
    advertisingData.setIncludePowerLevel(false);
    advertisingData.setLocalName(localName);
#if 0
    QList<QBluetoothUuid> serviceUuids;
    for (const auto &service : m_services) {
        serviceUuids.append(service->serviceUuid());
    }
    advertisingData.setServices(serviceUuids);
#else
    advertisingData.setServices(QList<QBluetoothUuid>() << AsteroidOSUuid);
#endif

    QLowEnergyAdvertisingParameters advertisingParameters{};
    advertisingParameters.setMode(QLowEnergyAdvertisingParameters::AdvInd);

    m_controller->startAdvertising(advertisingParameters, advertisingData, advertisingData);
}

void BluetoothService::onControllerStateChanged(QLowEnergyController::ControllerState state) {
    if (state == QLowEnergyController::UnconnectedState) {
        qDebug() << "Peripheral disconnected.";
    } else if (state == QLowEnergyController::AdvertisingState) {
        qDebug() << "Advertising started.";
    } else {
        qDebug() << "BluetoothService is in state " << state;
    }
}

void BluetoothService::onErrorOccurred(QLowEnergyController::Error error) {
    qWarning() << "Error occurred:" << error;
}

