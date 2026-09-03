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

#include "notifyingcharacteristic.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>

NotifyingCharacteristic::NotifyingCharacteristic(QDBusConnection bus, unsigned int index, QString uuid,
                                                  QStringList flags, Service *service,
                                                  QByteArray initialValue, QObject *parent)
    : Characteristic(bus, index, uuid, flags, service, parent), mValue(std::move(initialValue))
{
    connect(this, &NotifyingCharacteristic::valueChanged, this, &NotifyingCharacteristic::emitPropertiesChanged);
}

QByteArray NotifyingCharacteristic::getValue() const
{
    return mValue;
}

QByteArray NotifyingCharacteristic::ReadValue(QVariantMap)
{
    return mValue;
}

void NotifyingCharacteristic::StartNotify()
{
}

void NotifyingCharacteristic::StopNotify()
{
}

void NotifyingCharacteristic::setValue(const QByteArray &value)
{
    mValue = value;
    emit valueChanged();
}

void NotifyingCharacteristic::emitPropertiesChanged()
{
    QDBusConnection connection = QDBusConnection::systemBus();
    QDBusMessage message = QDBusMessage::createSignal(getPath().path(),
                                                      "org.freedesktop.DBus.Properties",
                                                      "PropertiesChanged");

    QVariantMap changedProperties;
    changedProperties[QStringLiteral("Value")] = QVariant(mValue);

    QList<QVariant> arguments;
    arguments << QVariant(GATT_CHRC_IFACE) << QVariant(changedProperties) << QVariant(QStringList());
    message.setArguments(arguments);

    if (!connection.send(message))
        qDebug() << "Failed to send DBus property notification signal";
}
