#include "ANCSService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QDebug>

ANCSService::ANCSService(BluetoothService &bluetoothService, QObject *parent);
    : QObject(parent)
    , m_controller(nullptr)
    , m_ancsService(nullptr)
{
}

void ANCSService::setController(QLowEnergyController *controller) {
    if (m_controller) {
        disconnect(m_controller, nullptr, this, nullptr);
        m_controller->deleteLater();
    }

    m_controller = controller;

    connect(m_controller, &QLowEnergyController::connected, this, [this]() {
        qDebug() << "Connected to iPhone";
        m_controller->discoverServices();
    });

    connect(m_controller, &QLowEnergyController::disconnected, this, [this]() {
        qDebug() << "Disconnected from iPhone";
        m_controller->deleteLater();
        m_controller = nullptr;
    });

    connect(m_controller, &QLowEnergyController::serviceDiscovered, this, &ANCSService::onServiceDiscovered);
    connect(m_controller, &QLowEnergyController::discoveryFinished, this, [this]() {
        if (!m_ancsService) {
            qDebug() << "ANCS service not found";
        }
    });

    m_controller->connectToDevice();
}

void ANCSService::onServiceDiscovered(const QBluetoothUuid &uuid) {
    if (uuid == QBluetoothUuid(QStringLiteral("7905F431-B5CE-4E99-A40F-4B1E122D00D0"))) {
        qDebug() << "ANCS service discovered!";
        discoverService();
    }
}

void ANCSService::discoverService() {
    if (m_ancsService) return;

    m_ancsService = m_controller->createServiceObject(QBluetoothUuid(QStringLiteral("7905F431-B5CE-4E99-A40F-4B1E122D00D0")), this);

    if (!m_ancsService) {
        qDebug() << "ANCS service not found!";
        return;
    }

    connect(m_ancsService, &QLowEnergyService::stateChanged, this, &ANCSService::onServiceStateChanged);
    connect(m_ancsService, &QLowEnergyService::characteristicChanged, this, &ANCSService::onCharacteristicChanged);

    m_ancsService->discoverDetails();
}

void ANCSService::onServiceStateChanged(QLowEnergyService::ServiceState state) {
    if (state == QLowEnergyService::ServiceDiscovered) {
        m_notificationSourceChar = m_ancsService->characteristic(QBluetoothUuid(QStringLiteral("9FBF120D-6301-42D9-8C58-25E699A21DBD")));
        m_controlPointChar = m_ancsService->characteristic(QBluetoothUuid(QStringLiteral("69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9")));
        m_dataSourceChar = m_ancsService->characteristic(QBluetoothUuid(QStringLiteral("22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB")));

        if (!m_notificationSourceChar.isValid() || !m_controlPointChar.isValid() || !m_dataSourceChar.isValid()) {
            qDebug() << "ANCS characteristics not found!";
            return;
        }

        subscribeToNotifications();
    }
}

void ANCSService::subscribeToNotifications() {
    if (!m_ancsService) return;

    m_ancsService->writeDescriptor(
        m_notificationSourceChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration),
        QByteArray::fromHex("0100")
    );

    m_ancsService->writeDescriptor(
        m_dataSourceChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration),
        QByteArray::fromHex("0100")
    );
}

void ANCSService::onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue) {
    if (characteristic.uuid() == m_notificationSourceChar.uuid()) {
        processNotificationSource(newValue);
    } else if (characteristic.uuid() == m_dataSourceChar.uuid()) {
        processDataSource(newValue);
    }
}

void ANCSService::onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value) {
    if (characteristic.uuid() == m_controlPointChar.uuid()) {
        qDebug() << "Control Point command sent:" << value;
    }
}

void ANCSService::processNotificationSource(const QByteArray &data) {
    qDebug() << "Notification Source Data received:" << data;
    emit notificationReceived(data);
}

void ANCSService::processDataSource(const QByteArray &data) {
    qDebug() << "Data Source Data received:" << data;
    emit dataSourceReceived(data);
}

