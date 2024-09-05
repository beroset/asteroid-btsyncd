#include "MediaService.h"
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyDescriptorData>
#include <QDebug>

#define MEDIA_COMMAND_PREVIOUS 0x0
#define MEDIA_COMMAND_NEXT     0x1
#define MEDIA_COMMAND_PLAY     0x2
#define MEDIA_COMMAND_PAUSE    0x3
#define MEDIA_COMMAND_VOLUME   0x4

static const QBluetoothUuid MediaServiceUuid{QString{"00007071-0000-0000-0000-00A57E401D05"}};
static const QBluetoothUuid MediaTitleUuid{QString{"00007001-0000-0000-0000-00A57E401D05"}};
static const QBluetoothUuid MediaAlbumUuid{QString{"00007002-0000-0000-0000-00A57E401D05"}};
static const QBluetoothUuid MediaArtistUuid{QString{"00007003-0000-0000-0000-00A57E401D05"}};
static const QBluetoothUuid MediaPlayingUuid{QString{"00007004-0000-0000-0000-00A57E401D05"}};
static const QBluetoothUuid MediaCommandUuid{QString{"00007005-0000-0000-0000-00A57E401D05"}};
static const QBluetoothUuid MediaVolumeUuid{QString{"00007006-0000-0000-0000-00A57E401D05"}};

MediaService::MediaService(BluetoothService &bluetoothService, QObject *parent) : QObject(parent) {
    m_player = new MprisPlayer(this);
    m_player->setServiceName("asteroid-btsyncd");
    m_player->setIdentity("Asteroid BLE Sync Daemon");

    m_player->setCanControl(true);
    m_player->setCanGoNext(true);
    m_player->setCanGoPrevious(true);
    m_player->setCanPause(true);
    m_player->setCanPlay(true);

    m_player->setCanSeek(false);
    m_player->setCanQuit(false);
    m_player->setCanRaise(false);
    m_player->setCanSetFullscreen(false);

    m_player->setPlaybackStatus(Mpris::Stopped);
    m_player->setLoopStatus(Mpris::None);
    m_player->setShuffle(false);

    QLowEnergyServiceData serviceData = createMediaServiceData();
    m_service = bluetoothService.addService(serviceData);
    connect(m_service, &QLowEnergyService::characteristicChanged, this, &MediaService::onCharacteristicWritten);
}

QLowEnergyService* MediaService::service() const {
    return m_service;
}

void MediaService::onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value) 
{
    QString receivedString = QString::fromUtf8(value);
    
    if (characteristic.uuid() == MediaTitleUuid) {
        qDebug() << "setting media Title to " << receivedString;
        QVariantMap metadata = m_player->metadata();
        metadata[Mpris::metadataToString(Mpris::Title)] = QString(value);
        m_player->setMetadata(metadata);
    } else if (characteristic.uuid() == MediaAlbumUuid) {
        qDebug() << "setting media Album to " << receivedString;
        QVariantMap metadata = m_player->metadata();
        metadata[Mpris::metadataToString(Mpris::Album)] = QString(value);
        m_player->setMetadata(metadata);
    } else if (characteristic.uuid() == MediaArtistUuid) {
        qDebug() << "setting media Artist to " << receivedString;
        QVariantMap metadata = m_player->metadata();
        metadata[Mpris::metadataToString(Mpris::Artist)] = QString(value);
        m_player->setMetadata(metadata);
    } else if (characteristic.uuid() == MediaPlayingUuid) {
        qDebug() << "setting media Playing to " << (bool(value[0]));
        m_player->setPlaybackStatus(value[0] ? Mpris::Playing : Mpris::Paused);
    } else if (characteristic.uuid() == MediaCommandUuid) {
        qDebug() << "setting media Command to " << (unsigned(value[0]));
        qDebug() << "size of passed QByteArray was " << value.size();
        QLowEnergyCharacteristic characteristic = m_service->characteristic(MediaCommandUuid);
        Q_ASSERT(characteristic.isValid());
        switch (value[0]) {
            case MEDIA_COMMAND_PREVIOUS:
            case MEDIA_COMMAND_NEXT:
            case MEDIA_COMMAND_PLAY:
            case MEDIA_COMMAND_PAUSE:
                m_service->writeCharacteristic(characteristic, QByteArray(1, value[0]));
                break;
            case MEDIA_COMMAND_VOLUME:
                m_service->writeCharacteristic(characteristic, QByteArray(value));
                break;
        }
    } else if (characteristic.uuid() == MediaVolumeUuid) {
        qDebug() << "setting media Volume to " << (int( (unsigned char) value[0])/100.0);
        m_player->setVolume(int( (unsigned char) value[0])/100.0);
    }
}

QLowEnergyServiceData MediaService::createMediaServiceData() {
    QLowEnergyCharacteristicData mediaTitleData;
    mediaTitleData.setUuid(MediaTitleUuid);
    mediaTitleData.setProperties(QLowEnergyCharacteristic::Write);
    mediaTitleData.setValue(QByteArray()); // empty value initially

    QLowEnergyCharacteristicData mediaAlbumData;
    mediaAlbumData.setUuid(MediaAlbumUuid);
    mediaAlbumData.setProperties(QLowEnergyCharacteristic::Write);
    mediaAlbumData.setValue(QByteArray()); // empty value initially

    QLowEnergyCharacteristicData mediaArtistData;
    mediaArtistData.setUuid(MediaArtistUuid);
    mediaArtistData.setProperties(QLowEnergyCharacteristic::Write);
    mediaArtistData.setValue(QByteArray()); // empty value initially

    QLowEnergyCharacteristicData mediaPlayingData;
    mediaPlayingData.setUuid(MediaPlayingUuid);
    mediaPlayingData.setProperties(QLowEnergyCharacteristic::Write);
    mediaPlayingData.setValue(QByteArray()); // empty value initially

    QLowEnergyCharacteristicData mediaVolumeData;
    mediaVolumeData.setUuid(MediaVolumeUuid);
    mediaVolumeData.setProperties(QLowEnergyCharacteristic::Write);
    mediaVolumeData.setValue(QByteArray()); // empty value initially

    QLowEnergyCharacteristicData mediaCommandData;
    mediaCommandData.setUuid(MediaCommandUuid);
    mediaCommandData.setProperties(QLowEnergyCharacteristic::Notify);
    mediaCommandData.setValue(QByteArray()); // empty value initially

    QLowEnergyDescriptorData cccd(QBluetoothUuid::ClientCharacteristicConfiguration, QByteArray(2, 0));
    mediaCommandData.addDescriptor(cccd);

    QLowEnergyServiceData serviceData;
    serviceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    serviceData.setUuid(MediaServiceUuid);
    serviceData.addCharacteristic(mediaTitleData);
    serviceData.addCharacteristic(mediaAlbumData);
    serviceData.addCharacteristic(mediaArtistData);
    serviceData.addCharacteristic(mediaPlayingData);
    serviceData.addCharacteristic(mediaVolumeData);
    serviceData.addCharacteristic(mediaCommandData);

    return serviceData;
}
