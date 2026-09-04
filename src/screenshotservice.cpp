/*
 * Copyright (C) 2017 - Florent Revest <revestflo@gmail.com>
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

#include <QDBusInterface>
#include <QDBusMessage>
#include <QDebug>
#include <QFile>

#include "screenshotservice.h"
#include "characteristic.h"

inline constexpr const char *SCREENSHOT_SERVICE_NAME = "org.nemomobile.lipstick";
inline constexpr const char *SCREENSHOT_MAIN_IFACE   = "org.nemomobile.lipstick";
inline constexpr const char *SCREENSHOT_PATH_BASE    = "/org/nemomobile/lipstick/screenshot";

inline constexpr const char *SCREENSH_UUID     = "00006071-0000-0000-0000-00A57E401D05";
inline constexpr const char *SCREENSH_REQ_UUID = "00006001-0000-0000-0000-00A57E401D05";
inline constexpr const char *SCREENSH_CON_UUID = "00006002-0000-0000-0000-00A57E401D05";

ScreenshotReqChrc::ScreenshotReqChrc(QDBusConnection bus, int index, Service *service)
    : Characteristic(bus, index, SCREENSH_REQ_UUID, {"encrypt-authenticated-write"}, service) {}

ScreenshotContentChrc::ScreenshotContentChrc(QDBusConnection bus, int index, Service *service)
    : NotifyingCharacteristic(bus, index, SCREENSH_CON_UUID,
                              {"encrypt-authenticated-read", "encrypt-authenticated-notify"}, service)
{}

void ScreenshotReqChrc::WriteValue(QByteArray, QVariantMap)
{
    QList<QVariant> argumentList;
    argumentList << "/tmp/btsyncd-screenshot.jpg";
    static QDBusInterface notifyApp(SCREENSHOT_SERVICE_NAME, SCREENSHOT_PATH_BASE, SCREENSHOT_MAIN_IFACE, QDBusConnection::systemBus());
    QDBusMessage reply = notifyApp.callWithArgumentList(QDBus::AutoDetect, "saveScreenshot", argumentList);
    if(reply.type() == QDBusMessage::ErrorMessage)
        fprintf(stderr, "ScreenshotReqChrc::WriteValue: D-Bus Error: %s\n", reply.errorMessage().toStdString().c_str());
    emit screenshotTaken("/tmp/btsyncd-screenshot.jpg");
}

void ScreenshotContentChrc::onScreenshotTaken(QString path)
{
    QFile f(path);
    if(!f.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open" << path;
        setValue(QByteArray::number(0x0));
        return;
    }

    qint64 totalSize = f.bytesAvailable();
    QByteArray sizeValue;
    sizeValue.append((totalSize >> 0) & 0xFF);
    sizeValue.append((totalSize >> 8) & 0xFF);
    sizeValue.append((totalSize >> 16) & 0xFF);
    sizeValue.append((totalSize >> 24) & 0xFF);
    setValue(sizeValue);

    while (!f.atEnd())
        setValue(f.read(20));
    f.close();
}

ScreenshotService::ScreenshotService(int index, QDBusConnection bus, QObject *parent) : Service(bus, index, SCREENSH_UUID, parent)
{
    ScreenshotReqChrc *reqChrc = new ScreenshotReqChrc(bus, 0, this);
    ScreenshotContentChrc *contChrc = new ScreenshotContentChrc(bus, 1, this);

    connect(reqChrc, SIGNAL(screenshotTaken(QString)), contChrc, SLOT(onScreenshotTaken(QString)));

    addCharacteristic(reqChrc);
    addCharacteristic(contChrc);
}

#include "serviceregistry.h"

REGISTER_SERVICE(ScreenshotService)
