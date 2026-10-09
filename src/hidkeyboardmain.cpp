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

#include <csignal>
#include <cstdio>
#include <memory>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QTimer>

#include "advertisement.h"
#include "bluezmanager.h"
#include "hidkeyboardapplication.h"
#include "keyboardinput.h"

namespace {

volatile std::sig_atomic_t shutdownRequested = 0;

void handleTerminationSignal(int)
{
    shutdownRequested = 1;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    std::signal(SIGTERM, handleTerminationSignal);
    std::signal(SIGINT, handleTerminationSignal);

    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus system bus.\n");
        return 3;
    }

    constexpr auto keyboardBusName = "org.asteroidos.HidKeyboard";
    if (!bus.registerService(keyboardBusName)) {
        qCritical() << "Cannot own D-Bus service" << keyboardBusName
                    << ":" << bus.lastError().message();
        return 1;
    }

    HidKeyboardApplication gattApplication(bus);
    Advertisement advertisement(
        {"00001812-0000-1000-8000-00805f9b34fb"}, bus);
    KeyboardInput keyboardInput(gattApplication.keyboardService());
    if (!bus.registerObject("/org/asteroidos/HidKeyboard", &keyboardInput,
                            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
        qCritical() << "Cannot export keyboard D-Bus API:" << bus.lastError().message();
        bus.unregisterService(keyboardBusName);
        return 1;
    }

    auto bluez = std::make_unique<BlueZManager>(gattApplication.getPath(), advertisement.getPath());
    QTimer shutdownPoll;
    QObject::connect(&shutdownPoll, &QTimer::timeout, &app, [&app] {
        if (shutdownRequested)
            app.quit();
    });
    shutdownPoll.start(100);

    const int result = app.exec();
    bluez.reset();
    bus.unregisterObject("/org/asteroidos/HidKeyboard");
    bus.unregisterService(keyboardBusName);
    return result;
}
