#include "ANCSService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QDBusInterface>
#include <QDBusArgument>
#include <QtCore/QLoggingCategory>
#include <QDebug>

static const QString ANCS_SERVICE_UUID = QStringLiteral("7905F431-B5CE-4E99-A40F-4B1E122D00D0");

static const QString ANCS_NOTIFICATION_SOURCE_CHARACTERISTIC_UUID = QStringLiteral("9FBF120D-6301-42D9-8C58-25E699A21DBD");
static const QString ANCS_CONTROL_POINT_CHARACTERISTIC_UUID = QStringLiteral("69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9");
static const QString ANCS_DATA_SOURCE_CHARACTERISTIC_UUID = QStringLiteral("22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB");

static const QString BLUEZ_SERVICE_NAME = QStringLiteral("org.bluez");
static const QString GATT_MANAGER_IFACE = QStringLiteral("org.bluez.GattManager1");
static const QString DEVICE_MANAGER_IFACE= QStringLiteral("org.bluez.Device1");
static const QString DBUS_OM_IFACE = QStringLiteral("org.freedesktop.DBus.ObjectManager");
static const QString GATT_CHRC_IFACE = QStringLiteral("org.bluez.GattCharacteristic1");

ANCSService::ANCSService(BluetoothService &bluetoothService, QObject *parent)
    : QObject(parent)
{
    connect(&bluetoothService, &BluetoothService::deviceConnected, this, &ANCSService::checkForANCS);
}

QLowEnergyService* ANCSService::service() const {
    return m_service;
}

static bool connectToANCSServer(QMap<QString, QVariantMap> dbusObject)
{
    bool connected{false};
    if (!dbusObject.contains(DEVICE_MANAGER_IFACE)) {
        return false;
    }
    qCDebug(btsyncd) << "Found DEVICE_MANAGER_IFACE";
    /*
     What we're looking for will look something like this:

     { QMap(
        ("org.bluez.Device1", QMap(
            ("Adapter", QVariant(QDBusObjectPath, ))
            ("Address", QVariant(QString, "98:69:8A:A7:E1:6A"))
            ("AddressType", QVariant(QString, "public"))
            ("Alias", QVariant(QString, "MyPhone"))
            ("Appearance", QVariant(ushort, 64))
            ("Blocked", QVariant(bool, false))
            ("Bonded", QVariant(bool, true))
            ("Connected", QVariant(bool, true))
            ("Icon", QVariant(QString, "phone"))
            ("LegacyPairing", QVariant(bool, false))
            ("Name", QVariant(QString, "MyPhone"))
            ("Paired", QVariant(bool, true))
            ("ServicesResolved", QVariant(bool, false))
            ("Trusted", QVariant(bool, false))
            ("UUIDs", QVariant(QStringList, ("00001800-0000-1000-8000-00805f9b34fb", "00001801-0000-1000-8000-00805f9b34fb", "00001805-0000-1000-8000-00805f9b34fb", "0000180a-0000-1000-8000-00805f9b34fb", "0000180f-0000-1000-8000-00805f9b34fb", "7905f431-b5ce-4e99-a40f-4b1e122d00d0", "89d3502b-0f36-433a-8ef4-c502ad55f8dc", "9fa480e0-4967-4542-9390-d343dc5d04ae", "d0611e78-bbb4-4591-a5f8-487910ae4366")))
        ))
        ("org.freedesktop.DBus.Introspectable", QMap())
        ("org.freedesktop.DBus.Properties", QMap())
        ) }

     */
    auto device{dbusObject[DEVICE_MANAGER_IFACE]};
    if (device.value(QStringLiteral("UUIDs")).toStringList().contains(ANCS_SERVICE_UUID, Qt::CaseInsensitive)) {
        qCDebug(btsyncd) << "Remote contains ANCS service";
        if (device[QStringLiteral("Connected")].toBool()) {
            qCDebug(btsyncd) << "we are connected";
            connected = true;
        } else {
            qCDebug(btsyncd) << "we are NOT connected";
        }
    }
    for (const auto& item : device) {
        qCDebug(btsyncd) << "item = " << item;
    }

    return connected;
}

static bool isMatchingCharacteristic(QString uuid, QMap<QString, QVariantMap> dbusObject)
{
    qCDebug(btsyncd) << "Comparison object is" << dbusObject;
    if (!dbusObject.contains(GATT_CHRC_IFACE)) {
        qCDebug(btsyncd) << "Did not contain GATT_CHRC_IFACE";
        return false;
    }
    QString charUuid = dbusObject.value(GATT_CHRC_IFACE).value(QStringLiteral("UUID")).toString();
    qCDebug(btsyncd) << "Comparing " << charUuid.toLower() << "with" << uuid.toLower() << ", result=" << (charUuid.toLower() == uuid.toLower());
    return charUuid.toLower() == uuid.toLower();
}

void ANCSService::checkForANCS()
{
    // gdbus call -y -d "org.bluez" -o /org/bluez/hci0/dev_98_69_8A_A7_E1_5B -m org.bluez.Device1.Connect
    // Although there is a connection made to the watch from another device,
    // we also need to connect in the other direction to allow subscribing to 
    // notifications.
    // Once we are connected, we can see if the remote device support ANCS.
    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusInterface remoteOm(BLUEZ_SERVICE_NAME, "/", DBUS_OM_IFACE, bus);
    QDBusMessage result = remoteOm.call("GetManagedObjects");
    //qCDebug(btsyncd) << "result={" << result << "}";
    QDBusObjectPath notificationChar;
    QDBusObjectPath controlChar;
    QDBusObjectPath dataChar;
    using InterfaceList = QMap<QString, QVariantMap>;
    const QDBusArgument argument = result.arguments().at(0).value<QDBusArgument>();
    if (argument.currentType() == QDBusArgument::MapType) {
        argument.beginMap();
        while (!argument.atEnd()) {
            QDBusObjectPath key;
            InterfaceList value;

            argument.beginMapEntry();
            argument >> key >> value;
            argument.endMapEntry();
            //qCDebug(btsyncd) << "key:{" << key << "}, value:{" << value << "}";
            if (connectToANCSServer(value)) {
                qCDebug(btsyncd) << "key:{" << key << "}, value:{" << value << "}";
            } 
            if (value.contains(GATT_CHRC_IFACE)) {
                if (isMatchingCharacteristic(ANCS_NOTIFICATION_SOURCE_CHARACTERISTIC_UUID, value)) {
                    qCDebug(btsyncd) << "Found ANCS notification source characteristic:" << key;
                    notificationChar = key;
                }
                else if (isMatchingCharacteristic(ANCS_CONTROL_POINT_CHARACTERISTIC_UUID, value)) {
                    qCDebug(btsyncd) << "Found ANCS control point characteristic:" << key;
                    controlChar = key;
                }
                else if (isMatchingCharacteristic(ANCS_DATA_SOURCE_CHARACTERISTIC_UUID, value)) {
                    qCDebug(btsyncd) << "Found ANCS data source characteristic:" << key;
                    dataChar = key;
                }
            }
        }
        argument.endMap();
    }
    if (!notificationChar.path().isEmpty() && !controlChar.path().isEmpty() && !dataChar.path().isEmpty()) {
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
