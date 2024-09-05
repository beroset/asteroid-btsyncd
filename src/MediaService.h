#ifndef MEDIASERVICE_H
#define MEDIASERVICE_H

#include <QObject>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyServiceData>
#include <MprisPlayer>
#include "BluetoothService.h"

class MediaService : public QObject {
    Q_OBJECT

public:
    explicit MediaService(BluetoothService &bluetoothService, QObject *parent = nullptr);
    QLowEnergyService* service() const;

private slots:
    void onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

private:
    QLowEnergyServiceData createMediaServiceData();
    QLowEnergyService *m_service;
    MprisPlayer *m_player;
    QByteArray m_value;
};

#endif // MEDIASERVICE_H

