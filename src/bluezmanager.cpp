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

#include "bluezmanager.h"

#include <QDBusServiceWatcher>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusArgument>
#include <QDebug>
#include <QTimer>

#include "common.h"

BlueZManager::BlueZManager(QDBusObjectPath appPath, QDBusObjectPath advertPath, QObject *parent)
    : QObject(parent), mAppPath(appPath), mAdvertPath(advertPath), mAdapter("adapter"), mBus(QDBusConnection::systemBus())
{
    mConnected = false;
    mServicesResolved = false;
    mConnectedDevice = "";

    mWatcher = new QDBusServiceWatcher(BLUEZ_SERVICE_NAME, QDBusConnection::systemBus());
    connect(mWatcher, SIGNAL(serviceRegistered(const QString&)), this, SLOT(serviceRegistered(const QString&)));
    connect(mWatcher, SIGNAL(serviceUnregistered(const QString&)), this, SLOT(serviceUnregistered(const QString&)));

    connect(this, SIGNAL(adapterChanged()), this, SLOT(onAdapterChanged()));
    connect(this, SIGNAL(connectedChanged()), this, SLOT(onConnectedChanged()));
    connect(this, SIGNAL(servicesResolvedChanged()), this, SLOT(onServicesResolvedChanged()));

    QDBusInterface remoteOm(BLUEZ_SERVICE_NAME, "/", DBUS_OM_IFACE, mBus);
    if(remoteOm.isValid())
        serviceRegistered(BLUEZ_SERVICE_NAME);
    else
        serviceUnregistered(BLUEZ_SERVICE_NAME);
}

void BlueZManager::serviceRegistered(const QString& name)
{
    qDebug() << "Service" << name << "is running";

    mBus.connect(BLUEZ_SERVICE_NAME, "/", DBUS_OM_IFACE, "InterfacesAdded", this, SLOT(InterfacesAdded(QDBusObjectPath, InterfaceList)));
    mBus.connect(BLUEZ_SERVICE_NAME, "/", DBUS_OM_IFACE, "InterfacesRemoved", this, SLOT(InterfacesRemoved(QDBusObjectPath, QStringList)));

    updateAdapter();
}

void BlueZManager::serviceUnregistered(const QString& name)
{
    qDebug() << "Service" << name << "is not running";
    setAdapter("adapter");
    mConnectedDevice = "";
    setConnected(false);
}

void BlueZManager::InterfacesAdded(QDBusObjectPath, InterfaceList)
{
    updateAdapter();
}

void BlueZManager::InterfacesRemoved(QDBusObjectPath, QStringList)
{
    updateAdapter();
}

void BlueZManager::updateAdapter() {
    QString adapter = "";
    bool connected = false;
    bool servicesResolved = false;

    QDBusInterface remoteOm(BLUEZ_SERVICE_NAME, "/", DBUS_OM_IFACE, mBus);
    QDBusMessage result = remoteOm.call("GetManagedObjects");

    const QDBusArgument argument = result.arguments().at(0).value<QDBusArgument>();
    if (argument.currentType() == QDBusArgument::MapType) {
        argument.beginMap();
        while (!argument.atEnd()) {
            QString key;
            InterfaceList value;

            argument.beginMapEntry();
            argument >> key >> value;
            argument.endMapEntry();

            if (value.contains(GATT_MANAGER_IFACE) && value.contains(LE_ADVERTISING_MANAGER_IFACE))
                adapter = key;

            if (value.contains(DEVICE_MANAGER_IFACE)) {
                 mBus.connect(BLUEZ_SERVICE_NAME, key, DBUS_PROPERTIES_IFACE, "PropertiesChanged", this, SLOT(PropertiesChanged(QString, QMap<QString, QVariant>, QStringList)));
                 QMap<QString, QVariant> properties = value.value(DEVICE_MANAGER_IFACE);
                 if(properties.contains("Connected") && properties.value("Connected").toBool()) {
                    connected = true;
                    if(properties.contains("Alias")) {
                       mConnectedDevice = properties.value("Alias").toString();
                    }
                 }

                 if(properties.contains("ServicesResolved"))
                    servicesResolved |= properties.value("ServicesResolved").toBool();

            }
        }
        argument.endMap();
    }

    setAdapter(adapter);
    setConnected(connected);
    setServicesResolved(servicesResolved);
}

void BlueZManager::setAdapter(QString adapter)
{
    if (adapter != mAdapter) {
        mAdapter = adapter;
        emit adapterChanged();
    }
}

void BlueZManager::setConnected(bool connected)
{
    if (connected != mConnected) {
        mConnected = connected;
        emit connectedChanged();
    }
}

void BlueZManager::setServicesResolved(bool servicesResolved)
{
    if (servicesResolved != mServicesResolved) {
        mServicesResolved = servicesResolved;
        emit servicesResolvedChanged();
    }
}

void BlueZManager::onAdapterChanged()
{
    if(mAdapter != "")
    {
        qDebug() << "BLE Adapter" << mAdapter << "found";

        QDBusInterface serviceManager(BLUEZ_SERVICE_NAME, mAdapter, GATT_MANAGER_IFACE, mBus);
        serviceManager.asyncCall("RegisterApplication", QVariant::fromValue(mAppPath), QVariantMap());
        qDebug() << "Service" << mAppPath.path() << "registered";

        QDBusInterface adManager(BLUEZ_SERVICE_NAME, mAdapter, LE_ADVERTISING_MANAGER_IFACE, mBus);
        adManager.asyncCall("RegisterAdvertisement", QVariant::fromValue(mAdvertPath), QVariantMap());
        qDebug() << "Advertisement" << mAdvertPath.path() << "registered";
    }
    else
        qDebug() << "No BLE adapter found";
}

void BlueZManager::PropertiesChanged(QString, QMap<QString, QVariant>, QStringList)
{
    updateAdapter();
}

void BlueZManager::onConnectedChanged()
{
    QString appName, appIcon, summary, body;
    appName = "";

    if(mConnected) {
        //% "Connected"
        summary = qtTrId("id-connected");
        body = mConnectedDevice;
        appIcon = "ios-bluetooth-outline";
    } else {
        mAncs.disconnect();
        mCts.disconnect();
        //% "Disconnected"
        summary = qtTrId("id-disconnected");
        body = "";
        appIcon = "ios-bluetooth-off-outline";
    }

    QVariantMap hints;
    hints.insert("x-nemo-preview-body", body);
    hints.insert("x-nemo-preview-summary", summary);
    hints.insert("x-nemo-feedback", "notif_silent");
    hints.insert("urgency", 3);
    hints.insert("transient", true);

    QList<QVariant> argumentList;
    argumentList << appName;
    argumentList << (uint) 0;
    argumentList << appIcon;
    argumentList << summary;
    argumentList << body;
    argumentList << QStringList();
    argumentList << hints;
    argumentList << (int) 3;

    static QDBusInterface notifyApp(NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH_BASE, NOTIFICATIONS_MAIN_IFACE);
    notifyApp.callWithArgumentList(QDBus::AutoDetect, "Notify", argumentList);
}

void BlueZManager::onServicesResolvedChanged() {
    if (mServicesResolved) {
        mAncs.searchForAncsCharacteristics();
        mCts.searchForTimeCharacteristics();
    }
}
