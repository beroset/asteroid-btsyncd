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

#ifndef NOTIFYINGCHARACTERISTIC_H
#define NOTIFYINGCHARACTERISTIC_H

#include "characteristic.h"

// A large fraction of this daemon's GATT characteristics share one shape:
// hold a single QByteArray value, serve it back via ReadValue(), and notify
// a subscribed central by sending BlueZ's PropertiesChanged D-Bus signal
// whenever the value is updated (StartNotify()/StopNotify() themselves are
// no-ops since we always keep the value current and always notify). Before
// this class existed, BatteryLvlChrc, ScreenshotContentChrc and
// MediaCommandsChrc each duplicated their own copy of that exact plumbing.
//
// A new "hold a value, notify on change" characteristic should derive from
// NotifyingCharacteristic and call setValue() whenever new data arrives,
// instead of re-implementing ReadValue/StartNotify/StopNotify/
// emitPropertiesChanged again.
class NotifyingCharacteristic : public Characteristic
{
    Q_OBJECT
    Q_PROPERTY(QByteArray Value READ getValue NOTIFY valueChanged)
public:
    explicit NotifyingCharacteristic(QDBusConnection bus, unsigned int index, QString uuid,
                                      QStringList flags, Service *service,
                                      QByteArray initialValue = QByteArray(),
                                      QObject *parent = nullptr);

    QByteArray getValue() const;

public slots:
    QByteArray ReadValue(QVariantMap options) override;
    void StartNotify() override;
    void StopNotify() override;

signals:
    void valueChanged();

protected:
    // Updates the stored value, emits valueChanged() and sends BlueZ's
    // PropertiesChanged D-Bus signal so any subscribed central is notified.
    // Always emits, even if the new value equals the previous one: several
    // characteristics (e.g. repeated media transport commands) rely on
    // every setValue() being observed as a distinct notification.
    void setValue(const QByteArray &value);

private:
    void emitPropertiesChanged();

    QByteArray mValue;
};

#endif // NOTIFYINGCHARACTERISTIC_H
