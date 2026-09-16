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

#ifndef ADVERTISEMENT_H
#define ADVERTISEMENT_H

#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QDBusConnection>

#define LE_ADVERTISEMENT_IFACE "org.bluez.LEAdvertisement1"

class Advertisement : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", LE_ADVERTISEMENT_IFACE)
    Q_PROPERTY(QString Type READ getType())
    Q_PROPERTY(QStringList ServiceUUIDs READ getServiceUUIDs())
    Q_PROPERTY(QStringList SolicitUUIDs READ getSolicitUUIDs())
    Q_PROPERTY(QMap<unsigned int, QByteArray> ManufacturerData READ getManufacturerData())
    Q_PROPERTY(QMap<QString, QByteArray> ServiceData READ getServiceData())
    Q_PROPERTY(bool IncludeTxPower READ getIncludeTxPower())
    Q_PROPERTY(bool Discoverable READ getDiscoverable())

public:
    explicit Advertisement(QDBusConnection bus = QDBusConnection::systemBus(), QObject *parent = nullptr);
    QDBusObjectPath getPath();

private:
    void addServiceData(QString uuid, QByteArray data);

    QString getType() const;
    QStringList getServiceUUIDs() const;
    QStringList getSolicitUUIDs() const;
    QMap<unsigned int, QByteArray> getManufacturerData() const;
    QMap<QString, QByteArray> getServiceData() const;
    bool getIncludeTxPower() const;
    bool getDiscoverable() const;

    QDBusConnection mBus;
    QString mPath = "/org/asteroidos/btsyncd/advertisement";
    QString mAdType = "peripheral";
    QStringList mServiceUuids = {{"00000000-0000-0000-0000-00a57e401d05"}};
    QStringList mSolicitUuids;
    QMap<unsigned int, QByteArray> mManufacturerData;
    QMap<QString, QByteArray> mServiceData;
    bool mIncludeTxPower = false;
    bool mDiscoverable = true;

public slots:
    void Release();
};

#endif // ADVERTISEMENT_H
