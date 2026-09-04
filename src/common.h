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

#ifndef COMMON_H
#define COMMON_H

/* D-Bus paths and interfaces */
/* These must be #define because they are each used in a Q_CLASSINFO macro */
#define BLUEZ_SERVICE_NAME "org.bluez"
#define DBUS_OM_IFACE      "org.freedesktop.DBus.ObjectManager"
#define GATT_CHRC_IFACE    "org.bluez.GattCharacteristic1"

/* These are not used in a Q_CLASSINFO macro */
inline constexpr const char *DBUS_PROPERTIES_IFACE = "org.freedesktop.DBus.Properties";

inline constexpr const char *NOTIFICATIONS_SERVICE_NAME = "org.freedesktop.Notifications";
inline constexpr const char *NOTIFICATIONS_MAIN_IFACE = "org.freedesktop.Notifications";
inline constexpr const char *NOTIFICATIONS_PATH_BASE = "/org/freedesktop/Notifications";

#endif // COMMON_H
