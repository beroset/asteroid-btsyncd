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

#include "bluezobjects.h"

#include <QDBusArgument>
#include <QDBusInterface>
#include <QDBusMessage>

namespace BluezObjects {

bool forEachManagedObject(const QDBusConnection &bus, const QString &service, const QString &path,
                           const std::function<void(const QString &objectPath,
                                                     const InterfaceList &interfaces)> &visitor)
{
    QDBusInterface remoteOm(service, path, "org.freedesktop.DBus.ObjectManager", bus);
    QDBusMessage result = remoteOm.call("GetManagedObjects");
    if (result.type() == QDBusMessage::ErrorMessage || result.arguments().isEmpty())
        return false;

    const QDBusArgument argument = result.arguments().at(0).value<QDBusArgument>();
    if (argument.currentType() != QDBusArgument::MapType)
        return true;

    argument.beginMap();
    while (!argument.atEnd()) {
        QString objectPath;
        InterfaceList interfaces;

        argument.beginMapEntry();
        argument >> objectPath >> interfaces;
        argument.endMapEntry();

        visitor(objectPath, interfaces);
    }
    argument.endMap();
    return true;
}

bool interfaceHasUuid(const InterfaceList &interfaces, const QString &ifaceName, const QString &uuid)
{
    if (!interfaces.contains(ifaceName))
        return false;
    QString ifaceUuid = interfaces.value(ifaceName).value("UUID").toString();
    return ifaceUuid.toLower() == uuid.toLower();
}

} // namespace BluezObjects
