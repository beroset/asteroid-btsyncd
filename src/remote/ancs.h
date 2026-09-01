#ifndef ANCS_H
#define ANCS_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QCache>
#include <QTimer>
#include <QDateTime>

#include "ancs_notification.h"
#include "remotecharacteristic.h"

// ANCS talks to the phone's Apple Notification Center Service, which is
// exposed by the *central* (the phone), not by us. It is a client of three
// RemoteCharacteristic instances (see remotecharacteristic.h for why this
// indirection exists) and otherwise only deals with ANCS protocol
// parsing/encoding of the QByteArray payloads it receives/sends.
class ANCS: public QObject
{
    Q_OBJECT
public:
    ANCS();
    void searchForAncsCharacteristics();
    void disconnect();
private slots:
    void onNotificationValueChanged(const QByteArray &bytes);
    void onDataValueChanged(const QByteArray &bytes);
    void EnableFeedbackForPastNotifications();

private:
    RemoteCharacteristic notificationCharacteristic;
    RemoteCharacteristic controlCharacteristic;
    RemoteCharacteristic dataCharacteristic;
    QCache<unsigned int, ANCSNotification> notificationCache;
    QDateTime currentSessionMaxTimestamp;
    QDateTime previousSessionMaxTimestamp;
    QTimer *pastNotificationsTimer;
    bool noFeedbackForPastNotifications;
    void appendByte(QByteArray &arr, unsigned int val);
    void append2Bytes(QByteArray &arr, unsigned int val);
    unsigned int decodeNumber(const QByteArray &arr, int offset, int length);
    int decodeStringAttribute(QByteArray bytes, int offset, int maxLenght, QString &result);
    void prepareQuery(QByteArray &result, const QByteArray &msgid);
    bool validateGetNotificationAttributesResponse(const QByteArray &bytes);
    void handleGetNotificationAttributesResponse(const QByteArray &bytes);
};

#endif // ANCS_H
