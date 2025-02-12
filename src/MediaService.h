#ifndef MEDIASERVICE_H
#define MEDIASERVICE_H

#include "asteroid-btsyncd.h"
#include <QObject>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyServiceData>
#ifndef DESKTOP_VERSION
#include <MprisPlayer>
#endif
#include "BluetoothService.h"

class MediaService : public QObject {
    Q_OBJECT

public:
    explicit MediaService(BluetoothService &bluetoothService, QObject *parent = nullptr);
    void add(BluetoothService &bluetoothService);
    void remove();
    QLowEnergyService* service() const;

public slots:
    void pauseRequested();
    void playRequested();
    void playPauseRequested();
    void stopRequested();
    void nextRequested();
    void previousRequested();
    void volumeRequested(double volume);

private slots:
    void onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

private:
    QLowEnergyServiceData createMediaServiceData();
    QLowEnergyService *m_service;
#ifndef DESKTOP_VERSION
    MprisPlayer *m_player;
#endif
    QByteArray m_value;
};

#endif // MEDIASERVICE_H

