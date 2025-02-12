#ifndef SCREENSHOTSERVICE_H
#define SCREENSHOTSERVICE_H

#include <QObject>
#include <QtBluetooth/QLowEnergyCharacteristic>
#include <QtBluetooth/QLowEnergyCharacteristicData>
#include <QtBluetooth/QLowEnergyServiceData>
#include "BluetoothService.h"

class ScreenshotService : public QObject {
    Q_OBJECT

public:
    explicit ScreenshotService(BluetoothService &bluetoothService, QObject *parent = nullptr);
    void add(BluetoothService &bluetoothService);
    void remove();
    QLowEnergyService* service() const;

private slots:
    void onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

private:
    QLowEnergyServiceData createScreenshotServiceData();
    QLowEnergyService *m_service;
};

#endif // SCREENSHOTSERVICE_H

