#include "BluetoothService.h"
#include <QtBluetooth/QLowEnergyAdvertisingData>
#include <QtBluetooth/QLowEnergyAdvertisingParameters>
#include <QDebug>

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
    advertisingData.setIncludePowerLevel(true);
    advertisingData.setLocalName(localName);

    QList<QBluetoothUuid> serviceUuids;
    for (const auto &service : m_services) {
        serviceUuids.append(service->serviceUuid());
    }
    advertisingData.setServices(serviceUuids);

    QLowEnergyAdvertisingParameters advertisingParameters;
    advertisingParameters.setMode(QLowEnergyAdvertisingParameters::AdvInd);

    m_controller->startAdvertising(advertisingParameters, advertisingData, advertisingData);
}

void BluetoothService::onControllerStateChanged(QLowEnergyController::ControllerState state) {
    if (state == QLowEnergyController::UnconnectedState) {
        qDebug() << "Peripheral disconnected.";
    } else if (state == QLowEnergyController::AdvertisingState) {
        qDebug() << "Advertising started.";
    }
}

void BluetoothService::onErrorOccurred(QLowEnergyController::Error error) {
    qWarning() << "Error occurred:" << error;
}

