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
#include <cerrno>
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

volatile std::sig_atomic_t signalPipeWriteFd = -1;

void handleTerminationSignal(int signal)
{
    const int savedErrno = errno;
    const unsigned char signalByte = static_cast<unsigned char>(signal);
    const int fd = signalPipeWriteFd;
    if (fd >= 0)
        (void)::write(fd, &signalByte, sizeof(signalByte));
    errno = savedErrno;
}

void disableTerminationSignals()
{
    signalPipeWriteFd = -1;
    std::signal(SIGTERM, SIG_IGN);
    std::signal(SIGINT, SIG_IGN);
}

class SignalPipe
{
public:
    bool open()
    {
        int fds[2];
        if (::pipe(fds) != 0)
            return false;

        for (int fd : fds) {
            const int statusFlags = ::fcntl(fd, F_GETFL);
            const int descriptorFlags = ::fcntl(fd, F_GETFD);
            if (statusFlags == -1 || descriptorFlags == -1
                || ::fcntl(fd, F_SETFL, statusFlags | O_NONBLOCK) == -1
                || ::fcntl(fd, F_SETFD, descriptorFlags | FD_CLOEXEC) == -1) {
                const int savedErrno = errno;
                ::close(fds[0]);
                ::close(fds[1]);
                errno = savedErrno;
                return false;
            }
        }
        mReadFd = fds[0];
        mWriteFd = fds[1];
        return true;
    }

    ~SignalPipe()
    {
        signalPipeWriteFd = -1;
        if (mReadFd >= 0)
            ::close(mReadFd);
        if (mWriteFd >= 0)
            ::close(mWriteFd);
    }

    int readFd() const { return mReadFd; }
    int writeFd() const { return mWriteFd; }

private:
    int mReadFd = -1;
    int mWriteFd = -1;
};

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    SignalPipe signalPipe;
    if (!signalPipe.open()) {
        perror("Cannot create or configure signal pipe");
        return 1;
    }
    signalPipeWriteFd = signalPipe.writeFd();
    std::signal(SIGTERM, handleTerminationSignal);
    std::signal(SIGINT, handleTerminationSignal);

    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus system bus.\n");
        disableTerminationSignals();
        return 3;
    }

    auto gattApplication = std::make_unique<HidKeyboardApplication>(bus);
    if (!gattApplication->isRegistered()) {
        disableTerminationSignals();
        return 1;
    }
    auto advertisement = std::make_unique<Advertisement>(
        QStringList{HID_KEYBOARD_SERVICE_UUID}, bus);
    auto keyboardInput = std::make_unique<KeyboardInput>(gattApplication->keyboardService());
    if (!bus.registerObject("/org/asteroidos/HidKeyboard", keyboardInput.get(),
                            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
        qCritical() << "Cannot export keyboard D-Bus API:" << bus.lastError().message();
        disableTerminationSignals();
        return 1;
    }

    constexpr auto keyboardBusName = "org.asteroidos.HidKeyboard";
    if (!bus.registerService(keyboardBusName)) {
        qCritical() << "Cannot own D-Bus service" << keyboardBusName
                    << ":" << bus.lastError().message();
        disableTerminationSignals();
        return 1;
    }

    auto bluez = std::make_unique<BlueZManager>(
        gattApplication->getPath(), advertisement->getPath());
    QObject::connect(gattApplication.get(), &QObject::destroyed,
                     bluez.get(), &BlueZManager::applicationDestroyed, Qt::DirectConnection);
    QObject::connect(advertisement.get(), &QObject::destroyed,
                     bluez.get(), &BlueZManager::advertisementDestroyed, Qt::DirectConnection);
    QSocketNotifier shutdownNotifier(signalPipe.readFd(), QSocketNotifier::Read, &app);
    QObject::connect(&shutdownNotifier, &QSocketNotifier::activated, &app,
                     [&app, readFd = signalPipe.readFd()] {
                         unsigned char signalByte;
                         while (::read(readFd, &signalByte, sizeof(signalByte)) > 0) {
                         }
                         app.quit();
                     });

    const int result = app.exec();
    shutdownNotifier.setEnabled(false);
    disableTerminationSignals();
    bluez->unregisterAdvertisement();
    bluez->unregisterApplication();
    bus.unregisterObject("/org/asteroidos/HidKeyboard");
    keyboardInput.reset();
    advertisement.reset();
    gattApplication.reset();
    bluez.reset();
    bus.unregisterService(keyboardBusName);
    return result;
}
