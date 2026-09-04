#include "ancs.h"

#include <QDebug>
#include <QDateTime>

#include "ancs_protocol_constants.h"
#include "gattbytes.h"

#define TITLE_MAX_LENGTH 50
#define MESSAGE_MAX_LENGTH 100
#define MAX_ANCS_NOTIFICATIONS 10
#define NO_FEEDBACK_FOR_PAST_NOTIFICATION_SECONDS 5

ANCS::ANCS() : notificationCharacteristic(ANCS_NOTIFICATION_SOURCE_CHARACTERISTIC_UUID),
    controlCharacteristic(ANCS_CONTROL_POINT_CHARACTERISTIC_UUID),
    dataCharacteristic(ANCS_DATA_SOURCE_CHARACTERISTIC_UUID),
    notificationCache(MAX_ANCS_NOTIFICATIONS), noFeedbackForPastNotifications(false)
{
    connect(&notificationCharacteristic, &RemoteCharacteristic::valueChanged,
            this, &ANCS::onNotificationValueChanged);
    connect(&dataCharacteristic, &RemoteCharacteristic::valueChanged,
            this, &ANCS::onDataValueChanged);

    pastNotificationsTimer = new QTimer(this);
    connect(pastNotificationsTimer, SIGNAL(timeout()), this,
            SLOT(EnableFeedbackForPastNotifications()));
    pastNotificationsTimer->setSingleShot(true);
    pastNotificationsTimer->setInterval(NO_FEEDBACK_FOR_PAST_NOTIFICATION_SECONDS * 1000);
}

void ANCS::search()
{
    qDebug() << "ANCS searching for characteristics";
    bool notificationFound = notificationCharacteristic.find();
    bool controlFound = controlCharacteristic.find();
    bool dataFound = dataCharacteristic.find();

    if (notificationFound && controlFound && dataFound) {
        qDebug() << "All ANCS characteristics found";
        notificationCharacteristic.startNotify();
        dataCharacteristic.startNotify();
        qDebug() << "ANCS notifications enabled";
        pastNotificationsTimer->start();
    }
}

void ANCS::disconnect()
{
    notificationCache.clear();
    previousSessionMaxTimestamp = currentSessionMaxTimestamp;
    currentSessionMaxTimestamp = QDateTime();
    noFeedbackForPastNotifications = true;
    pastNotificationsTimer->stop();
    notificationCharacteristic.stopNotify();
    dataCharacteristic.stopNotify();
}

void ANCS::appendByte(QByteArray &arr, unsigned int val)
{
    arr.append(static_cast<char>(val));
}

void ANCS::append2Bytes(QByteArray &arr, unsigned int val)
{
    appendByte(arr, val);
    appendByte(arr, val >> 8);
}

unsigned int ANCS::decodeNumber(const QByteArray &arr, int offset, int length)
{
    auto value = GattBytes::readLittleEndian(arr, offset, length);
    if (!value) {
        qWarning() << "ANCS: decodeNumber out of bounds (offset" << offset << "length" << length
                   << "in a" << arr.size() << "byte buffer)";
        return 0;
    }
    return *value;
}



void ANCS::prepareQuery(QByteArray &result, const QByteArray &msgId)
{
    appendByte(result, ANCS_COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES);
    result.append(msgId);
    appendByte(result, ANCS_NOTIFICATION_ATTRIBUTE_ID_TITLE);
    append2Bytes(result, TITLE_MAX_LENGTH);
    appendByte(result, ANCS_NOTIFICATION_ATTRIBUTE_ID_MESSAGE);
    append2Bytes(result, MESSAGE_MAX_LENGTH);
    appendByte(result, ANCS_NOTIFICATION_ATTRIBUTE_ID_DATE);
}

void ANCS::onNotificationValueChanged(const QByteArray &bytes)
{
    if (bytes.length() == 8) {
        unsigned int eventId = decodeNumber(bytes, 0, 1);
        if (eventId == ANCS_EVENT_ID_NOTIFICATION_ADDED || eventId == ANCS_EVENT_ID_NOTIFICATION_MODIFIED) {
            bool isNew = (eventId == ANCS_EVENT_ID_NOTIFICATION_ADDED);
            unsigned int eventFlags = decodeNumber(bytes, 1, 1);
            unsigned int categoryId = decodeNumber(bytes, 2, 1);
            QByteArray msgId = bytes.mid(4);
            unsigned int msgKey = decodeNumber(msgId, 0, 4);
            ANCSNotification *entry = notificationCache.object(msgKey);
            bool isNewEntry = !entry;
            if (isNewEntry)
                entry = new ANCSNotification;
            entry->eventFlags = eventFlags;
            entry->categoryId = categoryId;
            entry->isNew = isNew;
            // The cache owns its entries. For a MODIFIED event the key is
            // already present, so mutate that object in place; re-inserting
            // the same pointer would make QCache delete it first, leaving a
            // dangling pointer.
            if (isNewEntry)
                notificationCache.insert(msgKey, entry);

            QByteArray query;
            prepareQuery(query, msgId);
            controlCharacteristic.writeValue(query);
        } else if (eventId == ANCS_EVENT_ID_NOTIFICATION_REMOVED) {
            QByteArray msgId = bytes.mid(4);
            unsigned int msgKey = decodeNumber(msgId, 0, 4);
            notificationCache.remove(msgKey);
        }
    }
}

// return number of bytes processed or -1 on failure
int ANCS::decodeStringAttribute(QByteArray bytes, int offset, int maxLenght, QString &result)
{
    if (offset + 2 > bytes.length()) {
        qDebug() << "Premature end of message, ignoring";
        qDebug() << "Message was:" << bytes.toHex();
        return -1;
    }
    int length = decodeNumber(bytes, offset, 2);
    offset += 2;
    if (length > maxLenght) {
        qDebug() << "Unexpected length" << length << "ignoring message";
        qDebug() << "Message was:" << bytes.toHex();
        return -1;
    }
    if (offset + length > bytes.length()) {
        qDebug() << "Attribute spans beyond message length, ignoring";
        qDebug() << "Message was:" << bytes.toHex();
        return -1;
    }
    result = QString::fromUtf8(bytes.mid(offset, length));
    return length + 2;
}

void ANCS::onDataValueChanged(const QByteArray &bytes)
{
    if (bytes.length() < 1) {
        qDebug() << "Malformed message, length < 0, ignoring";
        return;
    }
    unsigned int commandId = decodeNumber(bytes, 0, 1);
    if (commandId == ANCS_COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES) {
        handleGetNotificationAttributesResponse(bytes);
    }
}

bool ANCS::validateGetNotificationAttributesResponse(const QByteArray &bytes)
{
    if (bytes.length() < 5) {
        qDebug() << "Malformed GetNotificationAttributes response, ignoring";
        qDebug() << "Message was:" << bytes.toHex();
        return false;
    }
    unsigned int commandId = decodeNumber(bytes, 0, 1);
    if (commandId != ANCS_COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES) {
        qDebug() << "Expected ANCS_COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES, ignoring";
        qDebug() << "Message was:" << bytes.toHex();
        return false;
    }
    return true;
}

void ANCS::handleGetNotificationAttributesResponse(const QByteArray &bytes)
{
    if (!validateGetNotificationAttributesResponse(bytes))
        return;
    QByteArray msgId = bytes.mid(1, 4);
    // skip event id and message id
    int offset = 5;
    QString title;
    QString message;
    QDateTime timestamp;
    while (offset < bytes.length()) {
        unsigned int attributeId = decodeNumber(bytes, offset, 1);
        offset += 1;
        if (attributeId == ANCS_NOTIFICATION_ATTRIBUTE_ID_TITLE) {
            int length = decodeStringAttribute(bytes, offset, TITLE_MAX_LENGTH, title);
            if (length == -1)
                return;
            offset += length;

        } else if (attributeId == ANCS_NOTIFICATION_ATTRIBUTE_ID_MESSAGE) {
            int length = decodeStringAttribute(bytes, offset, MESSAGE_MAX_LENGTH, message);
            if (length == -1)
                return;
            offset += length;
        } else if (attributeId == ANCS_NOTIFICATION_ATTRIBUTE_ID_DATE) {
            QString dateText;
            int length = decodeStringAttribute(bytes, offset, 15, dateText);
            if (length == -1)
                return;
            if (length > 0) {
                timestamp = QDateTime::fromString(dateText, "yyyyMMddTHHmmss");
                if (timestamp.isValid()) {
                    if (!currentSessionMaxTimestamp.isValid() || timestamp > currentSessionMaxTimestamp)
                        currentSessionMaxTimestamp = timestamp;
                }
            }
            offset += length;
        } else {
            qDebug() << "Unknown attribute id, ignoring whole message";
            qDebug() << "Message was:" << bytes.toHex();
            return;
        }
    }
    if (offset != bytes.length()) {
        qDebug() << "Message not fully processed, ignoring";
        qDebug() << "Message was:" << bytes.toHex();
        return;
    }
    unsigned int cacheKey = decodeNumber(msgId, 0, 4);
    ANCSNotification *notification = notificationCache.object(cacheKey);
    if (!notification) {
        qWarning() << "ANCS notification not found in the cache, skipping, key:" << cacheKey;
        return;
    }
    bool feedback = notification->isNew;
    if (noFeedbackForPastNotifications && previousSessionMaxTimestamp.isValid() && timestamp.isValid()
            && timestamp <= previousSessionMaxTimestamp)
        feedback = false;
    notification->title = title;
    notification->message = message;
    notification->refresh(feedback);
}

void ANCS::EnableFeedbackForPastNotifications()
{
    noFeedbackForPastNotifications = false;
}

#include "remotefeatureregistry.h"

REGISTER_REMOTE_FEATURE(ANCS)
