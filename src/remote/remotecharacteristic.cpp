/*
 * Copyright (C) 2026 - The asteroid-btsyncd contributors
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

#include "remotecharacteristic.h"

#include "bluezobjects.h"
#include "common.h"

#include <QDBusInterface>
#include <QDBusMessage>
#include <QDebug>

RemoteCharacteristic::RemoteCharacteristic(QString uuid, QDBusConnection bus, QObject *parent)
    : QObject(parent), mBus(bus), mUuid(uuid), mNotifying(false)
{
}

bool RemoteCharacteristic::find()
{
    QString foundPath;
    bool ok = BluezObjects::forEachManagedObject(mBus, BLUEZ_SERVICE_NAME, "/",
            [&](const QString &objectPath, const BluezObjects::InterfaceList &interfaces) {
        if (foundPath.isEmpty() && BluezObjects::interfaceHasUuid(interfaces, GATT_CHRC_IFACE, mUuid))
            foundPath = objectPath;
    });

    if (!ok) {
        qWarning() << "RemoteCharacteristic: GetManagedObjects failed while looking for" << mUuid;
        return false;
    }

    if (foundPath != mObjectPath) {
        // The object path changed (including disappearing, e.g. on a
        // disconnect, or reappearing elsewhere after a reconnect). Drop any
        // subscription tied to the *previous* path first, while mObjectPath
        // still refers to it, so stopNotify() disconnects the right D-Bus
        // signal and clears mNotifying. If we instead overwrote mObjectPath
        // first, a later stopNotify() would silently no-op on the new path
        // and a subsequent startNotify() call would then also no-op because
        // mNotifying would incorrectly still read true.
        if (mNotifying)
            stopNotify();
        mObjectPath = foundPath;
        if (!mObjectPath.isEmpty())
            qDebug() << "RemoteCharacteristic: found" << mUuid << "at" << mObjectPath;
    }
    return !foundPath.isEmpty();
}

bool RemoteCharacteristic::isAvailable() const
{
    return !mObjectPath.isEmpty();
}

QString RemoteCharacteristic::uuid() const
{
    return mUuid;
}

QByteArray RemoteCharacteristic::readValue(const QVariantMap &options)
{
    if (!isAvailable())
        return QByteArray();

    QDBusInterface characteristic(BLUEZ_SERVICE_NAME, mObjectPath, GATT_CHRC_IFACE, mBus);
    QDBusMessage response = characteristic.call("ReadValue", QVariant::fromValue(options));
    QList<QVariant> arguments = response.arguments();
    if (arguments.isEmpty() || arguments.first().userType() != QMetaType::QByteArray) {
        qWarning() << "RemoteCharacteristic: ReadValue on" << mUuid << "did not return a byte array";
        return QByteArray();
    }
    return arguments.first().toByteArray();
}

void RemoteCharacteristic::writeValue(const QByteArray &value, const QVariantMap &options)
{
    if (!isAvailable()) {
        qWarning() << "RemoteCharacteristic: writeValue on" << mUuid << "before it was found";
        return;
    }

    QDBusInterface characteristic(BLUEZ_SERVICE_NAME, mObjectPath, GATT_CHRC_IFACE, mBus);
    QDBusMessage response = characteristic.call("WriteValue", value, QVariant::fromValue(options));
    if (response.type() == QDBusMessage::ErrorMessage)
        qWarning() << "RemoteCharacteristic: WriteValue on" << mUuid << "failed:" << response.errorMessage();
}

void RemoteCharacteristic::startNotify()
{
    if (!isAvailable() || mNotifying)
        return;

    mBus.connect(BLUEZ_SERVICE_NAME, mObjectPath, DBUS_PROPERTIES_IFACE, "PropertiesChanged", this,
                 SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));

    QDBusInterface characteristic(BLUEZ_SERVICE_NAME, mObjectPath, GATT_CHRC_IFACE, mBus);
    characteristic.call("StartNotify");
    mNotifying = true;
}

void RemoteCharacteristic::stopNotify()
{
    if (!mNotifying)
        return;

    mBus.disconnect(BLUEZ_SERVICE_NAME, mObjectPath, DBUS_PROPERTIES_IFACE, "PropertiesChanged", this,
                    SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));

    if (isAvailable()) {
        QDBusInterface characteristic(BLUEZ_SERVICE_NAME, mObjectPath, GATT_CHRC_IFACE, mBus);
        characteristic.call("StopNotify");
    }
    mNotifying = false;
}

void RemoteCharacteristic::onPropertiesChanged(QString /* interfaceName */,
                                               QVariantMap changedProperties,
                                               QStringList /* invalidatedProperties */)
{
    if (!changedProperties.contains("Value"))
        return;
    QVariant value = changedProperties["Value"];
    if (value.userType() != QMetaType::QByteArray)
        return;
    emit valueChanged(value.toByteArray());
}
