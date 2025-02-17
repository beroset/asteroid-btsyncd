#include "Remote.h"
#include "BluetoothService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QDBusInterface>
#include <QDBusArgument>
#include <QtCore/QLoggingCategory>
#include <QDebug>


static const QString ANCS_NOTIFICATION_SOURCE_CHARACTERISTIC_UUID = QStringLiteral("9FBF120D-6301-42D9-8C58-25E699A21DBD");
static const QString ANCS_CONTROL_POINT_CHARACTERISTIC_UUID = QStringLiteral("69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9");
static const QString ANCS_DATA_SOURCE_CHARACTERISTIC_UUID = QStringLiteral("22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB");

static const QString BLUEZ_SERVICE_NAME = QStringLiteral("org.bluez");
static const QString GATT_MANAGER_IFACE = QStringLiteral("org.bluez.GattManager1");
static const QString DEVICE_MANAGER_IFACE= QStringLiteral("org.bluez.Device1");
static const QString DBUS_OM_IFACE = QStringLiteral("org.freedesktop.DBus.ObjectManager");
static const QString GATT_CHRC_IFACE = QStringLiteral("org.bluez.GattCharacteristic1");


Remote::Remote(const QBluetoothAddress &remoteDevice, const QBluetoothAddress &localDevice,
        QObject *parent)
    : QObject(parent)
    , m_local{localDevice}
    , m_remote{remoteDevice}
{
    // connect to DBus and find both the local and remote devices
    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusInterface remoteOm(BLUEZ_SERVICE_NAME, "/", DBUS_OM_IFACE, bus);
    QDBusMessage result = remoteOm.call(QStringLiteral("GetManagedObjects"));
    const QDBusArgument argument = result.arguments().at(0).value<QDBusArgument>();
    if (argument.currentType() == QDBusArgument::MapType) {
        argument.beginMap();
        while (!argument.atEnd()) {
            QDBusObjectPath key;
            InterfaceList dbusObject;

            argument.beginMapEntry();
            argument >> key >> dbusObject;
            argument.endMapEntry();
            // qCDebug(btsyncd) << "key:{" << key.path() << "}, dbusObject:{" << dbusObject << "}";
            if (dbusObject.contains(DEVICE_MANAGER_IFACE)) {
                auto device{dbusObject[DEVICE_MANAGER_IFACE]};
                    if (device.value(QStringLiteral("Connected")).toBool()) {
                    QBluetoothAddress discoveredAddress{device.value(QStringLiteral("Address")).toString()};
                    if (remoteDevice == discoveredAddress) {
                        qCDebug(btsyncd) << "remoteDevice:{" << remoteDevice  << "}";
                        remote_path = key.path();
                    }
                    else if (localDevice == discoveredAddress) {
                        qCDebug(btsyncd) << "localDevice:{" << localDevice << "}";
                        localDBusObject = dbusObject;
                    }
                    for (const auto &service : device.value(QStringLiteral("UUIDs")).toStringList()) {
                        QUuid uuid{service};
                        m_services.push_back(uuid);
                    }
                    qCDebug(btsyncd) << "\n\n" << device.value(QStringLiteral("Alias")) 
                        << " --> " << m_services << "\n";
                }
            }
        }
        argument.endMap();
    }
}

RemoteService *Remote::createServiceObject(const QBluetoothUuid &serviceUuid,
                                           QObject *parent)
{
    return new RemoteService{serviceUuid, parent};
}

Remote::~Remote()
{
    m_services.clear();
}

// Returns the list of services offered by the remote device
QList<QBluetoothUuid> Remote::services() const
{
    return m_services;
}

void Remote::discoverServices()
{
    for (const auto &svc : m_services) {
        emit serviceDiscovered(svc);
    }
}

// the local address
QBluetoothAddress Remote::localAddress() const
{
    return m_local;
}

// the remote address
QBluetoothAddress Remote::remoteAddress() const
{
    return m_remote;
}

QString Remote::remoteName() const
{
    return m_name;
}

QLowEnergyController::RemoteAddressType Remote::remoteAddressType() const
{
    return QLowEnergyController::RemoteAddressType::PublicAddress;
}

QLowEnergyController::Role Remote::role() const
{
    return QLowEnergyController::CentralRole;
}

QLowEnergyController::Error Remote::error() const
{
    return m_error;
}

// returns a string representation of the last error
QString Remote::errorString() const
{
    return QStringLiteral("No error");
}

QLowEnergyController::ControllerState Remote::state() const
{
    return m_state;
}
