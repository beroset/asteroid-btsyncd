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
   `src/remote/`, and are built on top of two small shared pieces:
   `RemoteCharacteristic` (`src/remote/remotecharacteristic.{h,cpp}`), which
   hides all of the BlueZ D-Bus plumbing (`GetManagedObjects` scanning,
   UUID matching, `PropertiesChanged` subscription, `StartNotify`/
   `ReadValue`/`WriteValue`); and `RemoteFeature`
   (`src/remote/remotefeature.h`), the common `search()`/`disconnect()`
   interface `BlueZManager` drives every reverse-direction feature through.
   See `src/remote/remotecharacteristic.h` for the full rationale. **Any new
   feature that needs to talk to a characteristic on the connected phone
   should use `RemoteCharacteristic` and implement `RemoteFeature` instead
   of re-implementing BlueZ D-Bus calls.**

Knowing which of these two directions your feature needs is the first and
most important design decision; the two examples below cover one of each.

## Where things get wired together

- `src/main.cpp` constructs one `Application` (the local GATT server root),
  one `Advertisement`, and one `BlueZManager`, then runs the Qt event loop.
- `Application` (`src/application.cpp`) owns the list of local `Service`
  objects and answers BlueZ's `GetManagedObjects` call for our own GATT
  tree. **A new peripheral-direction service self-registers** with the
  `REGISTER_SERVICE(MyService)` macro (see `src/serviceregistry.h`) placed
  in the service's own `.cpp` file; `Application` never needs editing.
- `BlueZManager` (`src/bluezmanager.{h,cpp}`) watches BlueZ over D-Bus,
  finds a usable adapter, registers our `Application` and `Advertisement`
  with it, and tracks whether a central is connected and whether
  `ServicesResolved` has become true for that connection.
  `BlueZManager::onServicesResolvedChanged()` is the trigger point for
  *reverse*-direction discovery: once the phone's own GATT services have
  been resolved, this is where every reverse-direction feature's
  `search()` is called, and `BlueZManager::onConnectedChanged()` is where
  every such feature is told to `disconnect()`/reset when the phone goes
  away. Both methods simply loop over `mRemoteFeatures`, a
  `std::vector<std::unique_ptr<RemoteFeature>>` (see
  `src/remote/remotefeature.h`) built once, at construction time, from
  `RemoteFeatureRegistry::instance().createAll()` — they do not call out to
  `ANCS`/`CTS`/etc. by name. **A new reverse-direction feature self-
  registers** with the `REGISTER_REMOTE_FEATURE(MyFeature)` macro (see
  `src/remote/remotefeatureregistry.h`) placed in the feature's own `.cpp`
  file; `BlueZManager` never needs editing.
- `src/common.h` holds all of the D-Bus interface name constants and all of
  this project's custom (non-standard) UUIDs, as `inline constexpr const
  char *` (not `#define`) so they're type-checked, scoped, and visible to a
  debugger. Standard Bluetooth SIG UUIDs
  (like the ones used below) are conventionally defined next to the class
  that uses them instead, since they are not `org.asteroidos`-specific.
- `NotifyingCharacteristic` (`src/notifyingcharacteristic.{h,cpp}`) is the
  base class for peripheral-direction characteristics that just hold one
  `QByteArray` value, serve it back via `ReadValue()`, and notify a
  subscribed phone whenever it changes. It supplies `ReadValue()`,
  `StartNotify()`/`StopNotify()` (no-ops — the value is always current), the
  `valueChanged()` signal and the BlueZ `PropertiesChanged` D-Bus plumbing,
  so a new "hold a value, notify on change" characteristic only needs to
  call `setValue()` — see `BatteryLvlChrc` (`src/batteryservice.{h,cpp}`)
  for the simplest example. Characteristics with different semantics (e.g.
  write-only, or no notification) still derive directly from
  `Characteristic`.

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
inline constexpr const char *BATTERY_UUID     = "0000180F-0000-1000-8000-00805f9b34fb";
inline constexpr const char *BATTERY_LVL_UUID = "00002a19-0000-1000-8000-00805f9b34fb";
```

### Step 1: create the class, in `src/remote/`

`src/remote/phonebattery.h`:

```cpp
#ifndef PHONEBATTERY_H
#define PHONEBATTERY_H

#include <QObject>

#include "remotecharacteristic.h"
#include "remotefeature.h"

// Reads the Battery Level characteristic (0x2A19) of the standard Battery
// Service (0x180F) exposed by the connected phone. See
// remotecharacteristic.h for why this talks to BlueZ directly instead of
// using QLowEnergyController.
class PhoneBattery : public QObject, public RemoteFeature
{
    Q_OBJECT
public:
    PhoneBattery();
    void search() override;
    void disconnect() override;

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

void PhoneBattery::search()
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
supplied by `RemoteCharacteristic`, and `search()`/`disconnect()` are the
two methods `RemoteFeature` requires `BlueZManager` be able to call
uniformly — so `PhoneBattery` only has to know the UUID to look for and how
to interpret the one-byte payload.

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

### Step 3: register it with `RemoteFeatureRegistry`

Instead of editing `bluezmanager.h`/`.cpp`, register `PhoneBattery` with
itself, at the bottom of `src/remote/phonebattery.cpp`:

```cpp
#include "remotefeatureregistry.h"

REGISTER_REMOTE_FEATURE(PhoneBattery)
```

`RemoteFeatureRegistry` (`src/remote/remotefeatureregistry.h`) is a small
self-registration mechanism: `PhoneBattery` must be default-constructible
and implement `RemoteFeature`, and this macro registers a factory for it as
a static-initialization side effect of linking `phonebattery.cpp` in.
`BlueZManager`'s constructor builds one instance of every registered
feature via `RemoteFeatureRegistry::instance().createAll()` and stores them
in `mRemoteFeatures`; `onServicesResolvedChanged()`/`onConnectedChanged()`
already loop over that list calling `search()`/`disconnect()`. No changes
to `bluezmanager.h`/`.cpp` are needed at all.

### Step 4: add it to the build

In `src/CMakeLists.txt`, add `remote/phonebattery.cpp` to `SRC` and
`remote/phonebattery.h` to `HEADERS`, next to the other `remote/*` entries.

That's the whole feature. No changes to `Application`, `Service`,
`Characteristic`, `Advertisement`, or `BlueZManager` are needed, because
the watch is not exposing anything new to the phone here — it's only
reading something the phone already exposes.

## Example 2 (peripheral direction): serving heart-rate data to the phone

Goal: the watch, acting as a BLE peripheral, wants to expose its own
heart-rate sensor readings to the phone using the standard Bluetooth
**Heart Rate Service** (`0x180D`) and **Heart Rate Measurement**
characteristic (`0x2A37`, notify-only). This is the ordinary direction that
`QLowEnergyController`/Qt Bluetooth would also support, and this codebase's
existing `BatteryService`/`BatteryLvlChrc` pair
(`src/batteryservice.{h,cpp}`) is the pattern to copy: a small "status"
class that owns the actual sensor/data source, and a
`NotifyingCharacteristic` subclass (`src/notifyingcharacteristic.h`) that
exposes its value over D-Bus/GATT and calls `setValue()` when it updates —
`NotifyingCharacteristic` takes care of `ReadValue()`,
`StartNotify()`/`StopNotify()` and sending BlueZ's `PropertiesChanged`
signal, so there is no `emitPropertiesChanged()`/`m_value` boilerplate to
write.

### Step 1: define the UUIDs

Standard Bluetooth SIG UUIDs for a specific, well-known service belong next
to the class that uses them (see how `BATTERY_UUID`/`BATTERY_LVL_UUID` in
`src/common.h` are used only by `batteryservice.cpp`); put these in
`heartrateservice.h` itself rather than adding project-specific-looking
`#define`s to `common.h`:

```cpp
inline constexpr const char *HEART_RATE_UUID      = "0000180D-0000-1000-8000-00805f9b34fb";
inline constexpr const char *HEART_RATE_MEAS_UUID = "00002a37-0000-1000-8000-00805f9b34fb";
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

#include "notifyingcharacteristic.h"
#include "service.h"

inline constexpr const char *HEART_RATE_UUID      = "0000180D-0000-1000-8000-00805f9b34fb";
inline constexpr const char *HEART_RATE_MEAS_UUID = "00002a37-0000-1000-8000-00805f9b34fb";

class HeartRateStatus;

class HeartRateMeasChrc : public NotifyingCharacteristic
{
    Q_OBJECT
public:
    HeartRateMeasChrc(QDBusConnection bus, int index, Service *service);

private slots:
    void onBpmChanged(int bpm);

private:
    HeartRateStatus *m_sensor;
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

#include <QDebug>

#include "heartrateservice.h"
#include "common.h"

HeartRateMeasChrc::HeartRateMeasChrc(QDBusConnection bus, int index, Service *service)
    : NotifyingCharacteristic(bus, index, HEART_RATE_MEAS_UUID, {"notify"}, service,
                              QByteArray(2, 0)) // flags=0, bpm=0 until first reading
{
    m_sensor = new HeartRateStatus(this);
    connect(m_sensor, &HeartRateStatus::bpmChanged,
            this, &HeartRateMeasChrc::onBpmChanged);
}

void HeartRateMeasChrc::onBpmChanged(int bpm)
{
    if (bpm < 0 || bpm > 255) {
        qWarning() << "Heart rate value out of uint8 range, ignoring:" << bpm;
        return;
    }
    QByteArray value(1, 0);        // flags: uint8 bpm format, no other fields
    value.append(static_cast<char>(bpm));
    setValue(value);
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

Instead of editing `application.cpp`, register `HeartRateService` with
itself, at the bottom of `src/heartrateservice.cpp`:

```cpp
#include "serviceregistry.h"

REGISTER_SERVICE(HeartRateService)
```

`ServiceRegistry` (`src/serviceregistry.h`) is a small self-registration
mechanism: `HeartRateService` must have the usual
`(int index, QDBusConnection bus, QObject *parent = 0)` constructor that
every `Service` subclass already has, and this macro registers a factory
for it as a static-initialization side effect of linking
`heartrateservice.cpp` in. `Application`'s constructor builds one instance
of every registered service — via `ServiceRegistry::instance().factories()`
— assigning indices in registration order; that order is unspecified across
translation units, but harmless, since a `Service`'s `index` is only ever
used to build a unique D-Bus object path (see `src/service.cpp`) and has no
other meaning. No changes to `application.h`/`.cpp` are needed at all.

### Step 5: add it to the build

In `src/CMakeLists.txt`, add `heartrateservice.cpp`/`heartratestatus.cpp` to
`SRC` and the two headers to `HEADERS`, next to `batteryservice.cpp`/
`batterystatus.cpp`.

No `Application` or `BlueZManager` changes are needed for this direction:
`HeartRateService` self-registers with `ServiceRegistry`, and the phone
discovers it on its own once BlueZ resolves our GATT tree — `BlueZManager`
only needs to know about *reverse*-direction features (and those
self-register too, via `RemoteFeatureRegistry`).

## Summary: which pattern to follow

| Your feature needs to...                                   | Follow the pattern in...                              | Lives in           |
|--------------------------------------------------------------|--------------------------------------------------------|---------------------|
| Serve a single value/notify it to the phone                   | `BatteryService`/`BatteryLvlChrc` via `NotifyingCharacteristic` | `src/*service.{h,cpp}` |
| Serve data/commands with other shapes (write-only, no notify) | `NotificationService`, `WeatherService`, `MediaService` (`Characteristic` directly) | `src/*service.{h,cpp}` |
| Read/write/subscribe to a characteristic the phone exposes    | `CTS`, `ANCS` via `RemoteCharacteristic` + `RemoteFeature` | `src/remote/*.{h,cpp}` |

When in doubt: if BlueZ's `GetManagedObjects` result for the *phone* would
contain the characteristic you care about, you want `RemoteCharacteristic`,
`RemoteFeature`, and `REGISTER_REMOTE_FEATURE` in `src/remote/`. If you're
inventing/exposing a characteristic that BlueZ should advertise for *our
own* adapter, you want a `Service`/`Characteristic` subclass registered
with `REGISTER_SERVICE`. Either way, adding the feature only requires its
own `.h`/`.cpp` files plus one line in `src/CMakeLists.txt` — no other
existing file needs to change.
