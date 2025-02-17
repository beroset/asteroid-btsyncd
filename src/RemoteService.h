#ifndef REMOTE_SERVICE_H
#define REMOTE_SERVICE_H
#include <QObject>
#include <QtBluetooth/QBluetoothUuid>

// TODO: create something that works like a QLowEnergyService class
class RemoteService : public QObject {
    Q_OBJECT
public:
    RemoteService(const QBluetoothUuid &serviceUuid, QObject *parent);
};
#endif // REMOTE_SERVICE_H
