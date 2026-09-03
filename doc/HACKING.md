# Hacking on asteroid-btsyncd

This document explains the architecture of `asteroid-btsyncd` well enough to
add a new feature, and works through two complete examples end to end.

## The two directions of communication

`asteroid-btsyncd` runs on the watch and talks to BlueZ over D-Bus. It never
uses Qt's `QLowEnergyController` class, because that class forces a single
BLE link into being *either* `CentralRole` *or* `PeripheralRole` for its
entire lifetime, and does not allow a peripheral-role connection to also
discover/read/write GATT characteristics exposed by the connected central.
This is a long-standing upstream limitation (see
[QTBUG-59925](https://bugreports.qt.io/browse/QTBUG-59925)) that is still
present in current Qt6 releases. BlueZ's own D-Bus API has no such
restriction: once the watch and the phone are paired and connected, either
side can act as a GATT client toward the other. So this codebase talks to
BlueZ directly, and as a result has two genuinely different kinds of
feature, living in two different places:

1. **Peripheral / "we serve GATT to the phone" features.** The watch
   advertises services and characteristics; the phone (as BLE central)
   discovers and reads/writes/subscribes to them. This is the "normal",
   Qt/QLowEnergyController-shaped direction. Files: `src/application.*`,
   `src/service.*`, `src/characteristic.*`, `src/descriptor.*`,
   `src/advertisement.*`, plus one file pair per service, e.g.
   `src/batteryservice.*`, `src/weatherservice.*`, `src/mediaservice.*`.

2. **Reverse / "we reach back into the phone's own GATT server" features.**
   The watch (still a BLE peripheral at the link layer) needs to read or
   subscribe to a characteristic that the *phone* exposes, e.g. ANCS
   notifications or the Current Time Service. These live under
   `src/remote/`, and are built on top of a single shared helper class,
   `RemoteCharacteristic` (`src/remote/remotecharacteristic.{h,cpp}`), which
   hides all of the BlueZ D-Bus plumbing (`GetManagedObjects` scanning,
   UUID matching, `PropertiesChanged` subscription, `StartNotify`/
   `ReadValue`/`WriteValue`). See `src/remote/remotecharacteristic.h` for the
   full rationale. **Any new feature that needs to talk to a characteristic
   on the connected phone should use `RemoteCharacteristic` instead of
   re-implementing BlueZ D-Bus calls.**

Knowing which of these two directions your feature needs is the first and
most important design decision; the two examples below cover one of each.

## Where things get wired together

- `src/main.cpp` constructs one `Application` (the local GATT server root),
  one `Advertisement`, and one `BlueZManager`, then runs the Qt event loop.
- `Application` (`src/application.cpp`) owns the list of local `Service`
  objects and answers BlueZ's `GetManagedObjects` call for our own GATT
  tree. **A new peripheral-direction service must be registered here** with
  `addService(new MyService(nextIndex, bus))`.
- `BlueZManager` (`src/bluezmanager.{h,cpp}`) watches BlueZ over D-Bus,
  finds a usable adapter, registers our `Application` and `Advertisement`
  with it, and tracks whether a central is connected and whether
  `ServicesResolved` has become true for that connection.
  `BlueZManager::onServicesResolvedChanged()` is the trigger point for
  *reverse*-direction discovery: once the phone's own GATT services have
  been resolved, this is where each reverse-direction feature's
  `searchForXyzCharacteristics()` is kicked off, and
  `BlueZManager::onConnectedChanged()` is where each such feature is told to
  `disconnect()`/reset when the phone goes away. **A new reverse-direction
  feature must be added here** as a member of `BlueZManager`, wired into
  both of these methods, alongside the existing `ANCS mAncs` and `CTS mCts`.
- `src/common.h` holds all of the D-Bus interface name constants and all of
  this project's custom (non-standard) UUIDs. Standard Bluetooth SIG UUIDs
  (like the ones used below) are conventionally defined next to the class
  that uses them instead, since they are not `org.asteroidos`-specific.

## Example 1 (reverse direction): reading the phone's battery level

Goal: the watch, acting as a BLE peripheral, wants to query the connected
phone (acting as BLE central) for its current battery level, using the
standard Bluetooth **Battery Service** (`0x180F`) and **Battery Level**
characteristic (`0x2A19`) that the phone's OS already exposes. This is
exactly the "peripheral reaches back to the central" case discussed above,
and follows the same shape as the existing `CTS` class
(`src/remote/cts.{h,cpp}`), which does this today for the phone's Current
Time Service.

Conveniently, `src/common.h` already defines these two UUIDs (they're
reused by the watch's own, peripheral-direction `BatteryService`):

```cpp
#define BATTERY_UUID      "0000180F-0000-1000-8000-00805f9b34fb"
#define BATTERY_LVL_UUID  "00002a19-0000-1000-8000-00805f9b34fb"
```

### Step 1: create the class, in `src/remote/`

`src/remote/phonebattery.h`:

```cpp
#ifndef PHONEBATTERY_H
#define PHONEBATTERY_H

#include <QObject>

#include "remotecharacteristic.h"

// Reads the Battery Level characteristic (0x2A19) of the standard Battery
// Service (0x180F) exposed by the connected phone. See
// remotecharacteristic.h for why this talks to BlueZ directly instead of
// using QLowEnergyController.
class PhoneBattery : public QObject
{
    Q_OBJECT
public:
    PhoneBattery();
    void searchForBatteryCharacteristic();
    void disconnect();

signals:
    // percentage is 0-100, as defined by the Battery Level characteristic.
    void levelChanged(int percentage);

private slots:
    void onLevelValueChanged(const QByteArray &bytes);

private:
    RemoteCharacteristic levelCharacteristic;
};

#endif // PHONEBATTERY_H
```

`src/remote/phonebattery.cpp`:

```cpp
#include "phonebattery.h"

#include <QDebug>

#include "common.h"

PhoneBattery::PhoneBattery() : levelCharacteristic(BATTERY_LVL_UUID)
{
    connect(&levelCharacteristic, &RemoteCharacteristic::valueChanged,
            this, &PhoneBattery::onLevelValueChanged);
}

void PhoneBattery::searchForBatteryCharacteristic()
{
    qDebug() << "PhoneBattery searching for characteristic";
    if (levelCharacteristic.find()) {
        qDebug() << "Phone battery level characteristic found";
        levelCharacteristic.startNotify();
        onLevelValueChanged(levelCharacteristic.readValue());
    }
}

void PhoneBattery::disconnect()
{
    levelCharacteristic.stopNotify();
}

void PhoneBattery::onLevelValueChanged(const QByteArray &bytes)
{
    // Battery Level is a single unsigned byte, 0-100.
    if (bytes.size() != 1) {
        qWarning() << "Unexpected battery level payload size" << bytes.size();
        return;
    }
    emit levelChanged(static_cast<unsigned char>(bytes.at(0)));
}
```

This is the entire BlueZ/D-Bus-facing part of the feature: `find()`,
`startNotify()`, `readValue()` and the `valueChanged` signal are all
supplied by `RemoteCharacteristic`, so `PhoneBattery` only has to know the
UUID to look for and how to interpret the one-byte payload.

### Step 2: decide what to do with the value

`levelChanged(int)` now fires whenever the phone's battery level is read or
changes. What you do with it depends on the goal — e.g. surface it in the
watch's system UI/settings the same way `WeatherService` publishes incoming
weather data via `MDConfItem` (see `src/weatherservice.cpp`):

```cpp
#include <MDConfItem>
...
void SomeConsumer::onPhoneBatteryLevelChanged(int percentage)
{
    MDConfItem("/org/asteroidos/phone/battery-level").set(percentage);
}
```

### Step 3: wire it into `BlueZManager`

In `src/bluezmanager.h`, add a member next to `mAncs`/`mCts`:

```cpp
#include "phonebattery.h"
...
private:
    ...
    PhoneBattery mPhoneBattery;
```

In `src/bluezmanager.cpp`:

```cpp
void BlueZManager::onServicesResolvedChanged() {
    if (mServicesResolved) {
        mAncs.searchForAncsCharacteristics();
        mCts.searchForTimeCharacteristics();
        mPhoneBattery.searchForBatteryCharacteristic();
    }
}
```

and in `onConnectedChanged()`, alongside the existing
`mAncs.disconnect(); mCts.disconnect();`:

```cpp
    mPhoneBattery.disconnect();
```

### Step 4: add it to the build

In `src/CMakeLists.txt`, add `remote/phonebattery.cpp` to `SRC` and
`remote/phonebattery.h` to `HEADERS`, next to the other `remote/*` entries.

That's the whole feature. No changes to `Application`, `Service`,
`Characteristic` or `Advertisement` are needed, because the watch is not
exposing anything new to the phone here — it's only reading something the
phone already exposes.

## Example 2 (peripheral direction): serving heart-rate data to the phone

Goal: the watch, acting as a BLE peripheral, wants to expose its own
heart-rate sensor readings to the phone using the standard Bluetooth
**Heart Rate Service** (`0x180D`) and **Heart Rate Measurement**
characteristic (`0x2A37`, notify-only). This is the ordinary direction that
`QLowEnergyController`/Qt Bluetooth would also support, and this codebase's
existing `BatteryService`/`BatteryLvlChrc` pair
(`src/batteryservice.{h,cpp}`) is the pattern to copy: a small "status"
class that owns the actual sensor/data source, and a `Characteristic`
subclass that exposes its value over D-Bus/GATT and emits
`PropertiesChanged` when it updates.

### Step 1: define the UUIDs

Standard Bluetooth SIG UUIDs for a specific, well-known service belong next
to the class that uses them (see how `BATTERY_UUID`/`BATTERY_LVL_UUID` in
`src/common.h` are used only by `batteryservice.cpp`); put these in
`heartrateservice.h` itself rather than adding project-specific-looking
`#define`s to `common.h`:

```cpp
#define HEART_RATE_UUID       "0000180D-0000-1000-8000-00805f9b34fb"
#define HEART_RATE_MEAS_UUID  "00002a37-0000-1000-8000-00805f9b34fb"
```

### Step 2: a status class that owns the sensor

`src/heartratestatus.h`/`.cpp`, following `src/batterystatus.{h,cpp}`
exactly: wrap whatever local API provides heart-rate samples (a D-Bus
signal from a sensor daemon, `QSensors`, etc.) and re-emit a plain Qt
signal:

```cpp
// heartratestatus.h
#ifndef HEARTRATESTATUS_H
#define HEARTRATESTATUS_H

#include <QObject>

class HeartRateStatus : public QObject
{
    Q_OBJECT
public:
    explicit HeartRateStatus(QObject *parent = nullptr);

signals:
    // bpm is 0 when no reading is available/contact is lost.
    void bpmChanged(int bpm);

private slots:
    void onSensorReading(int bpm);
};

#endif // HEARTRATESTATUS_H
```

The constructor subscribes to whichever local heart-rate sensor source this
target provides (mirroring how `BatteryStatus` subscribes to MCE's
`battery_level_ind` signal in `src/batterystatus.cpp`) and calls
`emit bpmChanged(bpm)` from `onSensorReading()`.

### Step 3: the GATT characteristic

`src/heartrateservice.h`:

```cpp
#ifndef HEARTRATESERVICE_H
#define HEARTRATESERVICE_H

#include <QObject>

#include "service.h"

#define HEART_RATE_UUID       "0000180D-0000-1000-8000-00805f9b34fb"
#define HEART_RATE_MEAS_UUID  "00002a37-0000-1000-8000-00805f9b34fb"

class HeartRateStatus;

class HeartRateMeasChrc : public Characteristic
{
    Q_OBJECT
    Q_PROPERTY(QByteArray Value READ getValue NOTIFY valueChanged)
public:
    HeartRateMeasChrc(QDBusConnection bus, int index, Service *service);

public slots:
    QByteArray ReadValue(QVariantMap) { return m_value; }
    void StartNotify() {}
    void StopNotify() {}

private slots:
    void emitPropertiesChanged();
    void onBpmChanged(int bpm);

signals:
    void valueChanged();

private:
    HeartRateStatus *m_sensor;
    QByteArray m_value;

    QByteArray getValue() { return m_value; }
};

class HeartRateService : public Service
{
    Q_OBJECT
public:
    explicit HeartRateService(int index, QDBusConnection bus, QObject *parent = 0);
};

#endif // HEARTRATESERVICE_H
```

`src/heartrateservice.cpp`, note the Heart Rate Measurement's mandatory
one-byte **flags** field (bit 0 clear = the heart-rate value that follows
is a `uint8`; the rest of the flags byte can stay `0` for a minimal
implementation — see the Bluetooth GATT Heart Rate Measurement
specification for the full bitfield if you want to add e.g. RR-interval
data later):

```cpp
#include "heartratestatus.h"

#include <QDBusMessage>
#include <QDebug>

#include "heartrateservice.h"
#include "characteristic.h"
#include "common.h"

HeartRateMeasChrc::HeartRateMeasChrc(QDBusConnection bus, int index, Service *service)
    : Characteristic(bus, index, HEART_RATE_MEAS_UUID, {"notify"}, service)
{
    m_sensor = new HeartRateStatus(this);
    connect(m_sensor, &HeartRateStatus::bpmChanged,
            this, &HeartRateMeasChrc::onBpmChanged);
    connect(this, SIGNAL(valueChanged()), this, SLOT(emitPropertiesChanged()));
    m_value = QByteArray(2, 0); // flags=0, bpm=0 until first reading
}

void HeartRateMeasChrc::onBpmChanged(int bpm)
{
    if (bpm < 0 || bpm > 255) {
        qWarning() << "Heart rate value out of uint8 range, ignoring:" << bpm;
        return;
    }
    m_value = QByteArray(1, 0);        // flags: uint8 bpm format, no other fields
    m_value.append(static_cast<char>(bpm));
    emit valueChanged();
}

void HeartRateMeasChrc::emitPropertiesChanged()
{
    QDBusConnection connection = QDBusConnection::systemBus();
    QDBusMessage message = QDBusMessage::createSignal(getPath().path(),
                                                      "org.freedesktop.DBus.Properties",
                                                      "PropertiesChanged");
    QVariantMap changedProperties;
    changedProperties[QStringLiteral("Value")] = QVariant(m_value);
    QList<QVariant> arguments;
    arguments << QVariant(GATT_CHRC_IFACE) << QVariant(changedProperties) << QVariant(QStringList());
    message.setArguments(arguments);
    if (!connection.send(message))
        qDebug() << "Failed to send DBus property notification signal";
}

HeartRateService::HeartRateService(int index, QDBusConnection bus, QObject *parent)
    : Service(bus, index, HEART_RATE_UUID, parent)
{
    addCharacteristic(new HeartRateMeasChrc(bus, 0, this));
}
```

Note `{"notify"}` (no `"read"`): real Heart Rate Measurement is
notify-only per the Bluetooth spec, unlike Battery Level which supports
both `"encrypt-authenticated-read"` and `"...-notify"`. Match the flags to
whatever the spec says for the characteristic you're implementing.

### Step 4: register the new service

In `src/application.cpp`:

```cpp
#include "heartrateservice.h"
...
    addService(new HeartRateService(6, bus)); // next free index after ScreenshotService/TimeService
```

(pick the next unused index; see the existing `addService()` calls for the
0-5 already in use).

### Step 5: add it to the build

In `src/CMakeLists.txt`, add `heartrateservice.cpp`/`heartratestatus.cpp` to
`SRC` and the two headers to `HEADERS`, next to `batteryservice.cpp`/
`batterystatus.cpp`.

No `BlueZManager` changes are needed for this direction: the phone
discovers this new service on its own once BlueZ resolves our GATT tree —
`BlueZManager` only needs to know about *reverse*-direction features.

## Summary: which pattern to follow

| Your feature needs to...                                   | Follow the pattern in...                              | Lives in           |
|--------------------------------------------------------------|--------------------------------------------------------|---------------------|
| Serve data/commands from the watch to the phone               | `BatteryService`/`BatteryLvlChrc` (or `NotificationService`, `WeatherService`, `MediaService`) | `src/*service.{h,cpp}` |
| Read/write/subscribe to a characteristic the phone exposes    | `CTS`, `ANCS` via `RemoteCharacteristic`                | `src/remote/*.{h,cpp}` |

When in doubt: if BlueZ's `GetManagedObjects` result for the *phone* would
contain the characteristic you care about, you want `RemoteCharacteristic`
and `src/remote/`. If you're inventing/exposing a characteristic that BlueZ
should advertise for *our own* adapter, you want a `Service`/`Characteristic`
subclass registered via `Application::addService()`.
