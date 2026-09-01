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

#ifndef BLUEZOBJECTS_H
#define BLUEZOBJECTS_H

#include <QDBusConnection>
#include <QMap>
#include <QString>
#include <QVariant>
#include <functional>

// Shared helpers for walking BlueZ's org.freedesktop.DBus.ObjectManager tree
// (GetManagedObjects). Both the "we are a peripheral" side (BlueZManager,
// looking for adapters/devices) and the "we are also acting like a central"
// side (RemoteCharacteristic, looking for GATT characteristics exposed by the
// connected phone) need to walk the very same reply shape. This file exists
// so that map-walking/demarshalling logic lives in exactly one place.
namespace BluezObjects {

// interface name -> (property name -> value), as returned for a single
// managed object by BlueZ's GetManagedObjects.
using InterfaceList = QMap<QString, QVariantMap>;

// Calls GetManagedObjects on the given D-Bus service/path and invokes
// visitor(objectPath, interfaces) once for every managed object found.
// Returns false if the D-Bus call itself failed (visitor is not called).
bool forEachManagedObject(const QDBusConnection &bus, const QString &service, const QString &path,
                           const std::function<void(const QString &objectPath,
                                                     const InterfaceList &interfaces)> &visitor);

// Convenience test: does this managed object implement ifaceName and does
// its "UUID" property (case-insensitively) match uuid? Used to find GATT
// characteristics/services by UUID.
bool interfaceHasUuid(const InterfaceList &interfaces, const QString &ifaceName, const QString &uuid);

} // namespace BluezObjects

#endif // BLUEZOBJECTS_H
