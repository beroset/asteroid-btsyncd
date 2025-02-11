# Introduction {#mainpage} #

This is the `asteroid-btsyncd` project which is the implementation of the comprehensive Bluetooth handler daemon for AsteroidOS.

## How it works
The `main.cpp` code creates an instance of the Device class and then runs the app, which means that everything after that is event-driven from the Device class.

When the Device class is constructed, it creates a BluetoothService class and then services for each of the specific services: BatteryService, HeartRateService, NotificationService, TimeService, MediaService, ScreenshotService, and WeatherService.

The BluetoothService creates a QLowEnergyController object and then makes the appropriate connections to that to enable the private BluetoothService::onControllerStateChanged slot.  When that slot is activated, the code checks the passed QLowEnergyController::ControllerState.  If it's QLowEnergyController::ConnectedState, it emits a deviceConnected signal with the local and remote addresses as passed arguments.

\startuml
main.cpp -> Device : create
Device -> BluetoothService : create
BluetoothService -> QLowEnergyController : create
QLowEnergyController -> BluetoothService : onControllerStateChanged(state)
BluetoothService -> Device : onDeviceConnected(remote, local)
Device -> Remote : create

QLowEnergyController -> BluetoothService : onControllerStateChanged(state)
BluetoothService -> Device : onDeviceDisconnected()
Device -> Remote : destroy
\enduml

Not shown in the diagram above is the creation of the individual services.
