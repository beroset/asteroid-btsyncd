#include "BluetoothService.h"
#include <QtBluetooth/QLowEnergyAdvertisingData>
#include <QtBluetooth/QLowEnergyAdvertisingParameters>
#include <QDebug>

static const QBluetoothUuid AsteroidOSUuid{QString{"00000000-0000-0000-0000-00a57e401d05"}};

BluetoothService::BluetoothService(QObject *parent) : QObject(parent) {
    m_controller = QLowEnergyController::createPeripheral(this);
    m_local = new QBluetoothLocalDevice(this);

    connect(m_controller, &QLowEnergyController::stateChanged, this, &BluetoothService::onControllerStateChanged);
    connect(m_controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this, &BluetoothService::onErrorOccurred);
    connect(m_local, &QBluetoothLocalDevice::pairingFinished, this, &BluetoothService::onPairingFinished);
}

void BluetoothService::reset() {
    // Delete existing objects
    if (m_controller) {
        disconnect(m_controller, nullptr, this, nullptr);
        m_controller->deleteLater();
    }
    if (m_local) {
        disconnect(m_local, nullptr, this, nullptr);
        m_local->deleteLater();
    }

    // Clear services list
    m_services.clear();

    // Create new instances
    m_controller = QLowEnergyController::createPeripheral(this);
    m_local = new QBluetoothLocalDevice(this);

    // Reconnect signals
    connect(m_controller, &QLowEnergyController::stateChanged, this, &BluetoothService::onControllerStateChanged);
    connect(m_controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error), this, &BluetoothService::onErrorOccurred);
    connect(m_local, &QBluetoothLocalDevice::pairingFinished, this, &BluetoothService::onPairingFinished);
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
    QList<QBluetoothUuid> serviceUuids;
    serviceUuids.append(AsteroidOSUuid);
    advertisingData.setServices(serviceUuids);

    QLowEnergyAdvertisingParameters advertisingParameters{};
    advertisingParameters.setMode(QLowEnergyAdvertisingParameters::AdvInd);

    m_controller->startAdvertising(advertisingParameters, advertisingData);
}

void BluetoothService::onControllerStateChanged(QLowEnergyController::ControllerState state) {
    if (state == QLowEnergyController::UnconnectedState) {
        qDebug() << "Peripheral disconnected.";
        reset();
        emit deviceDisconnected();
    } else if (state == QLowEnergyController::AdvertisingState) {
        qDebug() << "Advertising started.";
    } else if (state == QLowEnergyController::ConnectedState) {
        qDebug() << "Connected state";
        pairDevice(m_controller->remoteAddress());
        emit deviceConnected(m_controller->remoteAddress(), m_controller->localAddress());
    } else {
        qDebug() << "BluetoothService is in state " << state;
    }
}

void BluetoothService::pairDevice(const QBluetoothAddress &address) {
    if (m_local->pairingStatus(address) == QBluetoothLocalDevice::Paired ||
        m_local->pairingStatus(address) == QBluetoothLocalDevice::AuthorizedPaired) {
        return;
    }
    m_local->requestPairing(m_controller->remoteAddress(), QBluetoothLocalDevice::Paired);
}

void BluetoothService::onErrorOccurred(QLowEnergyController::Error error) {
    qWarning() << "Error occurred:" << error;
}

void BluetoothService::onPairingFinished(const QBluetoothAddress &address, QBluetoothLocalDevice::Pairing pairing) {
    qDebug() << "Pairing finished:" << address << pairing;
}

