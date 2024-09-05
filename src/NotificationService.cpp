#include "NotificationService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyDescriptorData>
#include <QDebug>
#include <QDBusInterface>
#include <QXmlStreamReader>
#include <QVariantMap>

static const QBluetoothUuid NotificationServiceUuid{QString{"00009071-0000-0000-0000-00A57E401D05"}};
static const QBluetoothUuid NotificationUpdateUuid{QString{"00009001-0000-0000-0000-00A57E401D05"}};
static const QBluetoothUuid NotificationFeedbackUuid{QString{"00009002-0000-0000-0000-00A57E401D05"}};
#define NOTIFICATIONS_SERVICE_NAME   "org.freedesktop.Notifications"
#define NOTIFICATIONS_MAIN_IFACE     "org.freedesktop.Notifications"
#define NOTIFICATIONS_PATH_BASE      "/org/freedesktop/Notifications"


NotificationService::NotificationService(BluetoothService &bluetoothService, QObject *parent) : QObject(parent) {
    QLowEnergyServiceData serviceData = createNotificationServiceData();
    m_service = bluetoothService.addService(serviceData);
    connect(m_service, &QLowEnergyService::characteristicChanged, this, &NotificationService::onCharacteristicWritten);
}

QLowEnergyService* NotificationService::service() const {
    return m_service;
}

void NotificationService::onNotificationClosed(uint replacesId, uint)
{
#if 0
    QLowEnergyCharacteristic characteristic = m_service->characteristic(QBluetoothUuid::NotificationLevel);
    Q_ASSERT(characteristic.isValid());
    m_service->writeCharacteristic(characteristic, QByteArray(1, percentage));
#endif
}

void NotificationService::onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value) {
    QString receivedString = QString::fromUtf8(value);
    qDebug() << "Notification received:" << receivedString;
    // emit notificationReceived(receivedString);
    QXmlStreamReader mReader;
    int id;
    uint replacesId;
    QString packageName, appName, appIcon, summary, body, vibrate = nullptr;

    mReader.addData(value);

    if(value.endsWith("</insert>")) {
        qDebug() << "Yes, this ends with </insert> !!";
        if(mReader.readNextStartElement() && mReader.name() == "insert") {
            while(mReader.readNextStartElement()) {
                if(mReader.name() == "pn") packageName = mReader.readElementText();
                else if(mReader.name() == "id") id = mReader.readElementText().toInt();
                else if(mReader.name() == "an") appName = mReader.readElementText();
                else if(mReader.name() == "ai") appIcon = mReader.readElementText();
                else if(mReader.name() == "su") summary = mReader.readElementText();
                else if(mReader.name() == "bo") body = mReader.readElementText();
                else if(mReader.name() == "vb") vibrate = mReader.readElementText();
                else mReader.skipCurrentElement();
            }
            mReader.clear();

            replacesId = mKnownAndroidNotifs.value(id, 0);

            QVariantMap hints;
            hints.insert("x-nemo-preview-body", body);
            hints.insert("x-nemo-preview-summary", summary);

            if (vibrate == nullptr) // for backwards compatibility
                hints.insert("x-nemo-feedback", "notif_strong");
            else if (vibrate.compare("none") == 0)
                hints.insert("x-nemo-feedback", "notif_silent");
            else if (vibrate.compare("normal") == 0)
                hints.insert("x-nemo-feedback", "notif_normal");
            else if (vibrate.compare("strong") == 0)
                hints.insert("x-nemo-feedback", "notif_strong");
            else if (vibrate.compare("ringtone") == 0)
                hints.insert("x-nemo-feedback", "ringtone");
            else
                hints.insert("x-nemo-feedback", "notif_strong");

            hints.insert("urgency", 3);

            QList<QVariant> argumentList;
            argumentList << appName;
            argumentList << replacesId;
            argumentList << appIcon;
            argumentList << summary;
            argumentList << body;
            argumentList << QStringList(); // actions
            argumentList << hints;
            argumentList << (int) 0;    // timeout

            qDebug() << argumentList;
            static QDBusInterface notifyApp(NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH_BASE, NOTIFICATIONS_MAIN_IFACE);
            QDBusMessage reply = notifyApp.callWithArgumentList(QDBus::AutoDetect, "Notify", argumentList);
            if(reply.type() == QDBusMessage::ErrorMessage) {
                fprintf(stderr, "NotificationsUpdateChrc::writeValue: D-Bus Error: %s\n", reply.errorMessage().toStdString().c_str());
            } else {
                qDebug() << "apparently sent the message with no error.";
            }

            if(!replacesId) {
                if(reply.arguments().size() > 0)
                    mKnownAndroidNotifs.insert(id, reply.arguments()[0].toUInt());
            }
        } else
            mReader.clear();
    } else if(value.endsWith("</removed>")) {
        if(mReader.readNextStartElement() && mReader.name() == "removed") {
            while(mReader.readNextStartElement())
                if(mReader.name() == "id") id = mReader.readElementText().toInt();
            mReader.clear();

            replacesId = mKnownAndroidNotifs.value(id, 0);
            if(replacesId) {
                QList<QVariant> argumentList;
                argumentList << replacesId;

                static QDBusInterface notifyApp(NOTIFICATIONS_SERVICE_NAME, NOTIFICATIONS_PATH_BASE, NOTIFICATIONS_MAIN_IFACE);
                QDBusMessage reply = notifyApp.callWithArgumentList(QDBus::AutoDetect, "CloseNotification", argumentList);
                if(reply.type() == QDBusMessage::ErrorMessage)
                    fprintf(stderr, "NotificationsUpdateChrc::writeValue: D-Bus Error: %s\n", reply.errorMessage().toStdString().c_str());

                mKnownAndroidNotifs.remove(replacesId);
            }
        } else
            mReader.clear();
    }
}

QLowEnergyServiceData NotificationService::createNotificationServiceData() {
    QLowEnergyCharacteristicData notificationUpdateData;
    notificationUpdateData.setUuid(NotificationUpdateUuid);
    notificationUpdateData.setProperties(QLowEnergyCharacteristic::Write);
    notificationUpdateData.setValue(QByteArray()); // empty value initially

    QLowEnergyCharacteristicData notificationFeedbackData;
    notificationFeedbackData.setUuid(NotificationFeedbackUuid);
    notificationFeedbackData.setProperties(QLowEnergyCharacteristic::Notify);

    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(NotificationServiceUuid);
    serviceData.addCharacteristic(notificationUpdateData);
    serviceData.addCharacteristic(notificationFeedbackData);

    return serviceData;
}

