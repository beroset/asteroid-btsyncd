#ifndef CTS_H
#define CTS_H
#include <QObject>
#include <QString>
#include <QMap>
#include <QVariant>
#include <QCache>
#include <QTimer>
#include <QDateTime>

#include "remotecharacteristic.h"
#include "remotefeature.h"

#define CTS_CHARACTERISTIC_UUID "00002a2b-0000-1000-8000-00805f9b34fb"

// CTS reads the Current Time Service exposed by the phone (the central), so
// like ANCS it is a client of a RemoteCharacteristic (see
// remotecharacteristic.h for why) rather than of QLowEnergyController. Its
// own job is limited to parsing the CTS byte layout, as per:
// https://www.bluetooth.com/specifications/specs/cts-1-1/
class CTS: public QObject, public RemoteFeature
{
    Q_OBJECT
    public:
        CTS();
        void search() override;
        void disconnect() override;
    private slots:
        void onTimeValueChanged(const QByteArray &bytes);
    private:
        RemoteCharacteristic timeCharacteristic;
        void parseCurrentTime(QByteArray& bytes);

};
#endif // CTS_H
