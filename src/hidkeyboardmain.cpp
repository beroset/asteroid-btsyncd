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
#include <fcntl.h>
#include <memory>
#include <unistd.h>

#include <QCoreApplication>
#include <QDBusError>
#include <QDBusConnection>
#include <QDebug>
#include <QSocketNotifier>

#include "advertisement.h"
#include "bluezmanager.h"
#include "hidkeyboardapplication.h"
#include "keyboardinput.h"

namespace {

int signalPipeWriteFd = -1;

void handleTerminationSignal(int signal)
{
    const unsigned char signalByte = static_cast<unsigned char>(signal);
    (void)::write(signalPipeWriteFd, &signalByte, sizeof(signalByte));
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    int signalPipe[2];
    if (::pipe(signalPipe) != 0) {
        perror("Cannot create signal pipe");
        return 1;
    }
    for (int fd : signalPipe) {
        const int statusFlags = ::fcntl(fd, F_GETFL);
        const int descriptorFlags = ::fcntl(fd, F_GETFD);
        if (statusFlags == -1 || descriptorFlags == -1
            || ::fcntl(fd, F_SETFL, statusFlags | O_NONBLOCK) == -1
            || ::fcntl(fd, F_SETFD, descriptorFlags | FD_CLOEXEC) == -1) {
            perror("Cannot configure signal pipe");
            ::close(signalPipe[0]);
            ::close(signalPipe[1]);
            return 1;
        }
    }
    signalPipeWriteFd = signalPipe[1];
    std::signal(SIGTERM, handleTerminationSignal);
    std::signal(SIGINT, handleTerminationSignal);

    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus system bus.\n");
        return 3;
    }

    auto gattApplication = std::make_unique<HidKeyboardApplication>(bus);
    auto advertisement = std::make_unique<Advertisement>(
        QStringList{HID_KEYBOARD_SERVICE_UUID}, bus);
    auto keyboardInput = std::make_unique<KeyboardInput>(gattApplication->keyboardService());
    if (!bus.registerObject("/org/asteroidos/HidKeyboard", keyboardInput.get(),
                            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
        qCritical() << "Cannot export keyboard D-Bus API:" << bus.lastError().message();
        return 1;
    }

    constexpr auto keyboardBusName = "org.asteroidos.HidKeyboard";
    if (!bus.registerService(keyboardBusName)) {
        qCritical() << "Cannot own D-Bus service" << keyboardBusName
                    << ":" << bus.lastError().message();
        return 1;
    }

    auto bluez = std::make_unique<BlueZManager>(
        gattApplication->getPath(), advertisement->getPath());
    QObject::connect(gattApplication.get(), &QObject::destroyed,
                     bluez.get(), &BlueZManager::unregisterApplication, Qt::DirectConnection);
    QObject::connect(advertisement.get(), &QObject::destroyed,
                     bluez.get(), &BlueZManager::unregisterAdvertisement, Qt::DirectConnection);
    QSocketNotifier shutdownNotifier(signalPipe[0], QSocketNotifier::Read, &app);
    QObject::connect(&shutdownNotifier, &QSocketNotifier::activated, &app, [&app, &signalPipe] {
        unsigned char signalByte;
        while (::read(signalPipe[0], &signalByte, sizeof(signalByte)) > 0) {
        }
        app.quit();
    });

    const int result = app.exec();
    bluez->unregisterAdvertisement();
    bluez->unregisterApplication();
    bus.unregisterObject("/org/asteroidos/HidKeyboard");
    keyboardInput.reset();
    advertisement.reset();
    gattApplication.reset();
    bluez.reset();
    bus.unregisterService(keyboardBusName);
    signalPipeWriteFd = -1;
    shutdownNotifier.setEnabled(false);
    ::close(signalPipe[0]);
    ::close(signalPipe[1]);
    return result;
}
