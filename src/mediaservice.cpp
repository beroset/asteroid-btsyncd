/*
 * Copyright (C) 2016 - Florent Revest <revestflo@gmail.com>
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

#include "mediaservice.h"
#include "characteristic.h"

#include <QDebug>
#include <QDBusMessage>
#include <MprisMetaData>

inline constexpr const char *MEDIA_UUID        = "00007071-0000-0000-0000-00A57E401D05";
inline constexpr const char *MEDIA_TITLE_UUID  = "00007001-0000-0000-0000-00A57E401D05";
inline constexpr const char *MEDIA_ALBUM_UUID  = "00007002-0000-0000-0000-00A57E401D05";
inline constexpr const char *MEDIA_ARTIST_UUID = "00007003-0000-0000-0000-00A57E401D05";
inline constexpr const char *MEDIA_PLAY_UUID   = "00007004-0000-0000-0000-00A57E401D05";
inline constexpr const char *MEDIA_COMM_UUID   = "00007005-0000-0000-0000-00A57E401D05";
inline constexpr const char *MEDIA_VOL_UUID    = "00007006-0000-0000-0000-00A57E401D05";

#define MEDIA_COMMAND_PREVIOUS 0x0
#define MEDIA_COMMAND_NEXT     0x1
#define MEDIA_COMMAND_PLAY     0x2
#define MEDIA_COMMAND_PAUSE    0x3
#define MEDIA_COMMAND_VOLUME   0x4

MediaCommandsChrc::MediaCommandsChrc(MprisPlayer *player, QDBusConnection bus, int index, Service *service)
    : NotifyingCharacteristic(bus, index, MEDIA_COMM_UUID, {"encrypt-authenticated-notify"}, service,
                              QByteArray(2, 0)),
      m_player(player)
{}

void MediaCommandsChrc::setCommand(char command)
{
    QByteArray value = getValue();
    value[0] = command;
    setValue(value);
}

class MediaTitleChrc : public Characteristic
{
public:
    MediaTitleChrc(MprisPlayer *player, QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, MEDIA_TITLE_UUID, {"encrypt-authenticated-write"}, service), m_player(player) {}

private:
    MprisPlayer *m_player;

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        m_player->metaData()->setTitle(QString(value));
    }
};

class MediaAlbumChrc : public Characteristic
{
public:
    MediaAlbumChrc(MprisPlayer *player, QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, MEDIA_ALBUM_UUID, {"encrypt-authenticated-write"}, service), m_player(player) {}

private:
    MprisPlayer *m_player;

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        m_player->metaData()->setAlbumTitle(QString(value));
    }
};

class MediaArtistChrc : public Characteristic
{
public:
    MediaArtistChrc(MprisPlayer *player, QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, MEDIA_ARTIST_UUID, {"encrypt-authenticated-write"}, service), m_player(player) {}

private:
    MprisPlayer *m_player;

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        m_player->metaData()->setContributingArtist(QString(value));
    }
};

class MediaPlayingChrc : public Characteristic
{
public:
    MediaPlayingChrc(MprisPlayer *player, QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, MEDIA_PLAY_UUID, {"encrypt-authenticated-write"}, service), m_player(player) {}

private:
    MprisPlayer *m_player;

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        if (!hasMinLength(value, 1))
            return;
        m_player->setPlaybackStatus(value[0] ? Mpris::Playing : Mpris::Paused);
    }
};

void MediaCommandsChrc::pauseRequested()
{
    setCommand(MEDIA_COMMAND_PAUSE);
}

void MediaCommandsChrc::playRequested()
{
    setCommand(MEDIA_COMMAND_PLAY);
}

void MediaCommandsChrc::playPauseRequested()
{
    setCommand(m_player->playbackStatus() == Mpris::Playing ? MEDIA_COMMAND_PAUSE : MEDIA_COMMAND_PLAY);
}

void MediaCommandsChrc::stopRequested()
{
    setCommand(MEDIA_COMMAND_PAUSE);
}

void MediaCommandsChrc::nextRequested()
{
    setCommand(MEDIA_COMMAND_NEXT);
}

void MediaCommandsChrc::previousRequested()
{
    setCommand(MEDIA_COMMAND_PREVIOUS);
}

void MediaCommandsChrc::volumeRequested(double volume)
{
    QByteArray value = getValue();
    value[0] = MEDIA_COMMAND_VOLUME;
    value[1] = static_cast<char>(volume * 100);
    setValue(value);
}

class MediaVolumeChrc : public Characteristic
{
public:
    MediaVolumeChrc(MprisPlayer *player, QDBusConnection bus, int index, Service *service)
        : Characteristic(bus, index, MEDIA_VOL_UUID, {"encrypt-authenticated-write"}, service), m_player(player) {}

private:
    MprisPlayer *m_player;

public slots:
    void WriteValue(QByteArray value, QVariantMap)
    {
        if (!hasMinLength(value, 1))
            return;
        m_player->setVolume(int( (unsigned char) value[0])/100.0);
    }
};

MediaService::MediaService(int index, QDBusConnection bus, QObject *parent) : Service(bus, index, MEDIA_UUID, parent)
{
    m_mprisPlayer = new MprisPlayer(this);
    m_mprisPlayer->setServiceName("asteroid-btsyncd");
    m_mprisPlayer->setIdentity("Asteroid BLE Sync Daemon");

    m_mprisPlayer->setCanControl(true);
    m_mprisPlayer->setCanGoNext(true);
    m_mprisPlayer->setCanGoPrevious(true);
    m_mprisPlayer->setCanPause(true);
    m_mprisPlayer->setCanPlay(true);

    m_mprisPlayer->setCanSeek(false);
    m_mprisPlayer->setCanQuit(false);
    m_mprisPlayer->setCanRaise(false);
    m_mprisPlayer->setCanSetFullscreen(false);

    m_mprisPlayer->setPlaybackStatus(Mpris::Stopped);
    m_mprisPlayer->setLoopStatus(Mpris::LoopNone);
    m_mprisPlayer->setShuffle(false);

    addCharacteristic(new MediaTitleChrc(m_mprisPlayer, bus, 0, this));
    addCharacteristic(new MediaAlbumChrc(m_mprisPlayer, bus, 1, this));
    addCharacteristic(new MediaArtistChrc(m_mprisPlayer, bus, 2, this));
    addCharacteristic(new MediaPlayingChrc(m_mprisPlayer, bus, 3, this));
    m_commandsChrc = new MediaCommandsChrc(m_mprisPlayer, bus, 4, this);
    addCharacteristic(m_commandsChrc);
    addCharacteristic(new MediaVolumeChrc(m_mprisPlayer, bus, 5, this));

    connect(m_mprisPlayer, SIGNAL(pauseRequested()), m_commandsChrc, SLOT(pauseRequested()));
    connect(m_mprisPlayer, SIGNAL(playRequested()), m_commandsChrc, SLOT(playRequested()));
    connect(m_mprisPlayer, SIGNAL(playPauseRequested()), m_commandsChrc, SLOT(playPauseRequested()));
    connect(m_mprisPlayer, SIGNAL(stopRequested()), m_commandsChrc, SLOT(stopRequested()));
    connect(m_mprisPlayer, SIGNAL(nextRequested()), m_commandsChrc, SLOT(nextRequested()));
    connect(m_mprisPlayer, SIGNAL(previousRequested()), m_commandsChrc, SLOT(previousRequested()));
    connect(m_mprisPlayer, SIGNAL(volumeRequested(double)), m_commandsChrc, SLOT(volumeRequested(double)));
 }

#include "serviceregistry.h"

REGISTER_SERVICE(MediaService)
