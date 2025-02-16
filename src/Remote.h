#ifndef REMOTE_H
#define REMOTE_H

#include <QObject>
#include <QtBluetooth/QLowEnergyService>
#include <QtBluetooth/QLowEnergyController>

/*
 * This class is a version of QLowEnergyController for a specific purpose.
 *
 * When a device, such as a phone, is connected via Bluetooth Low Energy
 * (BLE) to the watch, the phone is acting in the Central role, and the
 * watch is acting in the Peripheral role.
 *
 * For most purposes, this works just fine with Qt, but there are some
 * cases in which the QLowEnergyController class does not work.  One
 * such instance is when implementing the Apple Notification Center
 * Service.  This service allows the watch to display notifications
 * from the phone.  However, this requires a role reversal in which
 * the watch is Central and the phone is Peripheral and this is not
 * currently supported by the existing QLowEnergyController class.
 *
 * Another example is battery monitoring.  The watch implements a battery
 * service that allows a connected Central device to read the state of the
 * watch's battery.  However with this class it would also be possible for
 * the watch to monitor the state of the phone's battery as well.
 *
 */
class Remote : public QObject {
    Q_OBJECT

public:
    // create a Remote object that acts mostly like a QLowEnergyController
    explicit Remote(const QBluetoothAddress &remoteDevice,
                    const QBluetoothAddress &localDevice,
                    QObject *parent = nullptr);
    virtual ~Remote() override;
    // create an instance of the service represented by serviceUuid.
    QLowEnergyService *createServiceObject(const QBluetoothUuid &serviceUuid,
                                           QObject *parent = nullptr);
    // returns the last occurred error or NoError
    QLowEnergyController::Error error() const;
    // returns a string representation of the last error
    QString errorString() const;
    // the local address
    QBluetoothAddress localAddress() const;
    // the remote address
    QBluetoothAddress remoteAddress() const;
    // Returns the type of remoteAddress(). By default,
    // this value is initialized to PublicAddress.
    QLowEnergyController::RemoteAddressType remoteAddressType() const;
    // Returns the name of the remote Bluetooth Low Energy device
    QString remoteName() const;
    QLowEnergyController::Role role() const;
    // Returns the list of services offered by the remote device
    QList<QBluetoothUuid> services() const;
    // Returns the current state of the controller
    QLowEnergyController::ControllerState state() const;

signals:
    // This signal is emitted each time a new service is discovered
    void serviceDiscovered(const QBluetoothUuid &newService);
    // This signal is emitted when the controller's state changes.
    // The new state can also be retrieved via state()
    void stateChanged(QLowEnergyController::ControllerState state);

private slots:
#if 0
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
#endif

private:
    using InterfaceList = QMap<QString, QVariantMap>;
    QBluetoothAddress m_local;
    QBluetoothAddress m_remote;
    QLowEnergyController::ControllerState m_state =
            QLowEnergyController::ControllerState::UnconnectedState;
    QLowEnergyController::Error m_error = QLowEnergyController::Error::NoError;
    QList<QBluetoothUuid> m_services;
    QString m_name;
    QString remote_path;
    InterfaceList localDBusObject;
};
#endif // REMOTE_H

