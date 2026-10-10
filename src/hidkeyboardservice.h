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

#ifndef HIDKEYBOARDSERVICE_H
#define HIDKEYBOARDSERVICE_H

#include "service.h"

#include <QByteArray>

inline constexpr char HID_KEYBOARD_SERVICE_UUID[] = "00001812-0000-1000-8000-00805f9b34fb";

class HidKeyboardService : public Service
{
    Q_OBJECT
public:
    HidKeyboardService(int index, QDBusConnection bus, QObject *parent = nullptr);

    bool setKeyState(const QByteArray &state);
    QByteArray keyState() const;
    void setSuspended(bool suspended);
    bool bootProtocol() const;
    void setBootProtocol(bool bootProtocol);

signals:
    void outputReportChanged(quint8 leds);

private:
    class InputReport;
    QByteArray mKeyState = QByteArray(8, 0);
    bool mBootProtocol = false;
    bool mSuspended = false;
    InputReport *mReportInput = nullptr;
    InputReport *mBootInput = nullptr;

    void updateInputReports();
};

#endif // HIDKEYBOARDSERVICE_H
