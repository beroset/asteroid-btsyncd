#include "ScreenshotService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyDescriptorData>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QFile>
#include <QDebug>

#include <thread>

static const QBluetoothUuid ScreenshotServiceUuid{QString{"00006071-0000-0000-0000-00A57E401D05"}};
static const QBluetoothUuid ScreenshotRequestUuid{QString{"00006001-0000-0000-0000-00A57E401D05"}};
static const QBluetoothUuid ScreenshotContentUuid{QString{"00006002-0000-0000-0000-00A57E401D05"}};

static const QString SCREENSHOT_SERVICE_NAME = QStringLiteral("org.nemomobile.lipstick");
static const QString SCREENSHOT_MAIN_IFACE = QStringLiteral("org.nemomobile.lipstick");
static const QString SCREENSHOT_PATH_BASE = QStringLiteral("/org/nemomobile/lipstick/screenshot");

ScreenshotService::ScreenshotService(BluetoothService &bluetoothService, QObject *parent) : QObject(parent) {
    add(bluetoothService);
}

void ScreenshotService::add(BluetoothService &bluetoothService)
{
    QLowEnergyServiceData serviceData = createScreenshotServiceData();
    m_service = bluetoothService.addService(serviceData);
    connect(m_service, &QLowEnergyService::characteristicChanged, this, &ScreenshotService::onCharacteristicWritten);
}

void ScreenshotService::remove()
{
    disconnect(m_service, nullptr, this, nullptr);
}

QLowEnergyService* ScreenshotService::service() const {
    return m_service;
}

void ScreenshotService::onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value) 
{
    static const QString path{"/tmp/btsyncd-screenshot.jpg"};
    if (characteristic.uuid() == ScreenshotRequestUuid) {
        QList<QVariant> argumentList;
        argumentList << path;
        static QDBusInterface notifyApp(SCREENSHOT_SERVICE_NAME, SCREENSHOT_PATH_BASE, SCREENSHOT_MAIN_IFACE, QDBusConnection::systemBus());
        QDBusMessage reply = notifyApp.callWithArgumentList(QDBus::AutoDetect, "saveScreenshot", argumentList);
        if(reply.type() == QDBusMessage::ErrorMessage)
            fprintf(stderr, "ScreenshotReqChrc::WriteValue: D-Bus Error: %s\n", reply.errorMessage().toStdString().c_str());
    //    emit screenshotTaken("/tmp/btsyncd-screenshot.jpg");
        QFile f(path);
        if(!f.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open" << path;
            QLowEnergyCharacteristic characteristic = m_service->characteristic(ScreenshotContentUuid);
            Q_ASSERT(characteristic.isValid());
            m_service->writeCharacteristic(characteristic, QByteArray(1, 0));
            return;
        }

        qint64 totalSize = f.bytesAvailable();
        QByteArray m_value{}; 
        m_value.append((totalSize >> 0) & 0xFF);
        m_value.append((totalSize >> 8) & 0xFF);
        m_value.append((totalSize >> 16) & 0xFF);
        m_value.append((totalSize >> 24) & 0xFF);
        QLowEnergyCharacteristic characteristic = m_service->characteristic(ScreenshotContentUuid);
        Q_ASSERT(characteristic.isValid());
        m_service->writeCharacteristic(characteristic, m_value);

        while (!f.atEnd()) {
            static const unsigned mtu{20};
            m_value = f.read(mtu);
            m_service->writeCharacteristic(characteristic, m_value);
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(mtu * 180us);
        }
        f.close();
    }
}

QLowEnergyServiceData ScreenshotService::createScreenshotServiceData() {
    QLowEnergyCharacteristicData screenshotRequestData;
    screenshotRequestData.setUuid(ScreenshotRequestUuid);
    screenshotRequestData.setProperties(QLowEnergyCharacteristic::WriteNoResponse);
    screenshotRequestData.setValue(QByteArray()); // empty value initially

    QLowEnergyCharacteristicData screenshotContentData;
    screenshotContentData.setUuid(ScreenshotContentUuid);
    screenshotContentData.setProperties(QLowEnergyCharacteristic::Read | QLowEnergyCharacteristic::Notify);
    screenshotContentData.setValue(QByteArray()); // empty value initially

    QLowEnergyDescriptorData cccd(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
    screenshotContentData.addDescriptor(cccd);

    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(ScreenshotServiceUuid);
    serviceData.addCharacteristic(screenshotRequestData);
    serviceData.addCharacteristic(screenshotContentData);

    return serviceData;
}

