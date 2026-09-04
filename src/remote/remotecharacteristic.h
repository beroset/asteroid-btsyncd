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

#ifndef REMOTECHARACTERISTIC_H
#define REMOTECHARACTERISTIC_H

// Why this class exists
// ----------------------
// asteroid-btsyncd acts as a BLE *peripheral*: it advertises and serves GATT
// services to the phone (see application.h/service.h/characteristic.h). But a
// couple of features - ANCS notifications and the Current Time Service - need
// the opposite direction: the watch has to discover and read/write GATT
// characteristics that the *phone* (the central) exposes.
//
// Qt's QLowEnergyController deliberately does not support this. A controller
// is created via either createCentral() or createPeripheral() and is locked
// into that QLowEnergyController::Role for its lifetime: discoverServices(),
// services() and createServiceObject() only work in CentralRole, while
// addService()/startAdvertising() only work in PeripheralRole. There is no
// supported way for a single controller - or a peripheral-role connection -
// to also enumerate and use the remote device's own GATT services. This is a
// long-standing upstream limitation (see QTBUG-59925) that is still present
// in the newest Qt6 releases; the class documentation itself still hedges
// with "this limitation may be removed in future releases".
//
// BlueZ and the Bluetooth spec place no such restriction on a connected,
// paired link: once paired, either side can act as a GATT client toward the
// other over the same connection. So, rather than working around Qt's
// internal, undocumented structures (or waiting for upstream Qt to add this),
// this class talks to BlueZ directly over D-Bus for exactly this one
// "reach back to the central device" need, while everything else in this
// codebase (the peripheral/GATT-server side) is untouched and does not need
// this class at all.
//
// RemoteCharacteristic is the single, shared implementation of that pattern.

#include <QByteArray>
#include <QDBusConnection>
#include <QObject>
#include <QString>
#include <QVariantMap>

class RemoteCharacteristic : public QObject
{
    Q_OBJECT
public:
    explicit RemoteCharacteristic(QString uuid, QDBusConnection bus = QDBusConnection::systemBus(),
                                   QObject *parent = nullptr);

    // Scans GetManagedObjects on the BlueZ service for a GATT characteristic
    // whose UUID matches ours (typically exposed by the currently connected,
    // paired central device). Returns true if found. Safe to call again,
    // e.g. after a reconnect, to re-resolve the object path.
    bool find();

    // True once find() has located the characteristic's D-Bus object path.
    bool isAvailable() const;

    QString uuid() const;

    // Synchronously reads the characteristic's current value.
    QByteArray readValue(const QVariantMap &options = QVariantMap());

    // Writes a value to the characteristic (e.g. the ANCS control point).
    void writeValue(const QByteArray &value, const QVariantMap &options = QVariantMap());

    // Subscribes/unsubscribes to Value change notifications. Once notify is
    // enabled, valueChanged() is emitted for every update.
    void startNotify();
    void stopNotify();

signals:
    void valueChanged(const QByteArray &value);

private slots:
    void onPropertiesChanged(QString interfaceName, QVariantMap changedProperties,
                             QStringList invalidatedProperties);

private:
    QDBusConnection mBus;
    QString mUuid;
    QString mObjectPath;
    bool mNotifying;
};

#endif // REMOTECHARACTERISTIC_H
