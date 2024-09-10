#include "ANCSService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QDBusInterface>
#include <QDBusArgument>
#include <QtCore/QLoggingCategory>
#include <QDebug>

#define ANCS_SERVICE_UUID "7905F431-B5CE-4E99-A40F-4B1E122D00D0"

#define ANCS_NOTIFICATION_SOURCE_CHARACTERISTIC_UUID "9FBF120D-6301-42D9-8C58-25E699A21DBD"
#define ANCS_CONTROL_POINT_CHARACTERISTIC_UUID "69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9"
#define ANCS_DATA_SOURCE_CHARACTERISTIC_UUID "22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB"

#define BLUEZ_SERVICE_NAME           "org.bluez"
#define GATT_MANAGER_IFACE           "org.bluez.GattManager1"
#define DEVICE_MANAGER_IFACE         "org.bluez.Device1"
#define DBUS_OM_IFACE                "org.freedesktop.DBus.ObjectManager"
#define GATT_CHRC_IFACE              "org.bluez.GattCharacteristic1"

ANCSService::ANCSService(BluetoothService &bluetoothService, QObject *parent)
    : QObject(parent)
{
    connect(&bluetoothService, &BluetoothService::deviceConnected, this, &ANCSService::checkForANCS);
}

QLowEnergyService* ANCSService::service() const {
    return m_service;
}

static bool isMatchingCharacteristic(QString uuid, QMap<QString, QVariantMap> dbusObject)
{
    qCDebug(btsyncd) << "Comparison object is" << dbusObject;
    if (!dbusObject.contains(GATT_CHRC_IFACE)) {
        qCDebug(btsyncd) << "Did not contain GATT_CHRC_IFACE";
//        return false;
    }
    QString charUuid = dbusObject.value(GATT_CHRC_IFACE).value("UUID").toString();
    qCDebug(btsyncd) << "Comparing " << charUuid.toLower() << "with" << uuid.toLower() << ", result=" << (charUuid.toLower() == uuid.toLower());
    return charUuid.toLower() == uuid.toLower();
}

void ANCSService::checkForANCS()
{
    // gdbus call -y -d "org.bluez" -o /org/bluez/hci0/dev_98_69_8A_A7_E1_5B -m org.bluez.Device1.Connect
    // Although there is a connection made to the watch from another device,
    // we also need to connect in the other direction to allow subscribing to 
    // notifications.
    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusInterface remoteOm(BLUEZ_SERVICE_NAME, "/", DBUS_OM_IFACE, bus);
    QDBusMessage result = remoteOm.call("GetManagedObjects");
    QString notificationChar;
    QString controlChar;
    QString dataChar;
    const QDBusArgument argument = result.arguments().at(0).value<QDBusArgument>();
    if (argument.currentType() == QDBusArgument::MapType) {
        argument.beginMap();
        while (!argument.atEnd()) {
            QString key;
            QMap<QString, QVariantMap> value;

            argument.beginMapEntry();
            argument >> key >> value;
            argument.endMapEntry();
            qCDebug(btsyncd) << "key:{" << key << "}, value:{" << value << "}";
            if (isMatchingCharacteristic(ANCS_NOTIFICATION_SOURCE_CHARACTERISTIC_UUID, value)) {
                qCDebug(btsyncd) << "Found ANCS notification source characteristic:" << key;
                notificationChar = key;
            }
            if (isMatchingCharacteristic(ANCS_CONTROL_POINT_CHARACTERISTIC_UUID, value)) {
                qCDebug(btsyncd) << "Found ANCS control point characteristic:" << key;
                controlChar = key;
            }
            if (isMatchingCharacteristic(ANCS_DATA_SOURCE_CHARACTERISTIC_UUID, value)) {
                qCDebug(btsyncd) << "Found ANCS data source characteristic:" << key;
                dataChar = key;
            }
        }
        argument.endMap();
    }
    if (!notificationChar.isEmpty() && !controlChar.isEmpty() && !dataChar.isEmpty()) {
        qCDebug(btsyncd) << "All ANCS characteristics found";
#if 0
        controlCharacteristic = controlChar;
        bus.connect(BLUEZ_SERVICE_NAME, notificationChar, DBUS_PROPERTIES_IFACE, "PropertiesChanged", this,
                    SLOT(NotificationCharacteristicPropertiesChanged(QString, QMap<QString, QVariant>, QStringList)));
        bus.connect(BLUEZ_SERVICE_NAME, dataChar, DBUS_PROPERTIES_IFACE, "PropertiesChanged", this,
                    SLOT(DataCharacteristicPropertiesChanged(QString, QMap<QString, QVariant>, QStringList)));
        QDBusInterface dataCharacteristicIface("org.bluez", dataChar, GATT_CHRC_IFACE,
                                               QDBusConnection::systemBus());
        QDBusInterface notificationCharacteristicIface("org.bluez", notificationChar, GATT_CHRC_IFACE,
                                                       QDBusConnection::systemBus());
        dataCharacteristicIface.call("StartNotify");
        notificationCharacteristicIface.call("StartNotify");
        qCDebug(btsyncd) << "ANCS notifications enabled";
        pastNotificationsTimer->start();
#endif
    }
}

#if 0
void ANCSService::setController(QLowEnergyController *controller) {
    if (m_controller) {
        disconnect(m_controller, nullptr, this, nullptr);
        m_controller->deleteLater();
    }

    m_controller = controller;

    connect(m_controller, &QLowEnergyController::connected, this, [this]() {
        qCDebug(btsyncd) << "Connected to iPhone";
        m_controller->discoverServices();
    });

    connect(m_controller, &QLowEnergyController::disconnected, this, [this]() {
        qCDebug(btsyncd) << "Disconnected from iPhone";
        m_controller->deleteLater();
        m_controller = nullptr;
    });

    connect(m_controller, &QLowEnergyController::serviceDiscovered, this, &ANCSService::onServiceDiscovered);
    connect(m_controller, &QLowEnergyController::discoveryFinished, this, [this]() {
        if (!m_ancsService) {
            qCDebug(btsyncd) << "ANCS service not found";
        }
    });

    m_controller->connectToDevice();
}

void ANCSService::onServiceDiscovered(const QBluetoothUuid &uuid) {
    if (uuid == QBluetoothUuid(QStringLiteral("7905F431-B5CE-4E99-A40F-4B1E122D00D0"))) {
        qCDebug(btsyncd) << "ANCS service discovered!";
        discoverService();
    }
}

void ANCSService::discoverService() {
    if (m_ancsService) return;

    m_ancsService = m_controller->createServiceObject(QBluetoothUuid(QStringLiteral("7905F431-B5CE-4E99-A40F-4B1E122D00D0")), this);

    if (!m_ancsService) {
        qCDebug(btsyncd) << "ANCS service not found!";
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
            qCDebug(btsyncd) << "ANCS characteristics not found!";
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
        qCDebug(btsyncd) << "Control Point command sent:" << value;
    }
}

void ANCSService::processNotificationSource(const QByteArray &data) {
    qCDebug(btsyncd) << "Notification Source Data received:" << data;
    emit notificationReceived(data);
}

void ANCSService::processDataSource(const QByteArray &data) {
    qCDebug(btsyncd) << "Data Source Data received:" << data;
    emit dataSourceReceived(data);
}

#endif
