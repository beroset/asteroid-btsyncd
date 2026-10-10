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

#include "hidkeyboardapplication.h"

#include <QDebug>
#include <QDBusMetaType>
#include <QDBusError>

#include "descriptor.h"

HidKeyboardApplication::HidKeyboardApplication(QDBusConnection bus)
    : mService(new HidKeyboardService(0, bus, this))
{
    qDBusRegisterMetaType<InterfaceList>();
    qDBusRegisterMetaType<ManagedObjectList>();
    mRegistered = bus.registerObject(mPath, this, QDBusConnection::ExportAllSlots);
    if (!mRegistered)
        qCritical() << "Cannot register HID GATT application:" << bus.lastError().message();
}

QDBusObjectPath HidKeyboardApplication::getPath() const
{
    return QDBusObjectPath(mPath);
}

HidKeyboardService *HidKeyboardApplication::keyboardService() const
{
    return mService;
}

bool HidKeyboardApplication::isRegistered() const
{
    return mRegistered;
}

ManagedObjectList HidKeyboardApplication::GetManagedObjects()
{
    ManagedObjectList response;

    QVariantMap serviceProperties;
    serviceProperties.insert("UUID", mService->getUuid());
    serviceProperties.insert("Primary", mService->getPrimary());
    serviceProperties.insert("Characteristics", QVariant::fromValue(mService->getCharacteristicPaths()));
    response[mService->getPath()].insert(GATT_SERVICE_IFACE, serviceProperties);

    for (Characteristic *characteristic : mService->getCharacteristics()) {
        QVariantMap characteristicProperties;
        characteristicProperties.insert("Service", QVariant::fromValue(characteristic->getService()));
        characteristicProperties.insert("UUID", characteristic->getUuid());
        characteristicProperties.insert("Flags", characteristic->getFlags());
        characteristicProperties.insert("Descriptors", QVariant::fromValue(characteristic->getDescriptorPaths()));
        response[characteristic->getPath()].insert(GATT_CHRC_IFACE, characteristicProperties);

        for (Descriptor *descriptor : characteristic->getDescriptors()) {
            QVariantMap descriptorProperties;
            descriptorProperties.insert("UUID", descriptor->getUuid());
            descriptorProperties.insert("Characteristic", QVariant::fromValue(descriptor->getCharacteristic()));
            descriptorProperties.insert("Flags", descriptor->getFlags());
            response[descriptor->getPath()].insert(GATT_DESC_IFACE, descriptorProperties);
        }
    }

    return response;
}
