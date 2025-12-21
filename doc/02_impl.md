# Implementation notes {#implementation} #

This page contains notes about how this implementation was created, which includes some hints about debugging and testing.

## Overview

The `asteroid-btsyncd` software interfaces via D-Bus with an underlying BlueZ instance and acts as a Peripheral device.  This is supported well enough by the Qt classes, but in the case of ANCS and some other possible new facilities, the Qt classes do not support Bluetooth completely.  Working around this was the reason for the introduction of the Remote class, but that is not yet completely implemented.

## Status as of Febrary 2025

What I would have liked would have been to use the Qt BLE facilities to rewrite and make things simpler in the future.  However, one major limitation of the existing QLowEnergyController class is that it can either be a Peripheral device (which the watch is) or can be a Central device (which the watch usually isn't) but not both at the same time.  This means that it was pretty easy to port 99% of it, but there are exactly two features where that doesn't work: ANCS (the Apple notifications) and the recently added time synchronization.

The reason is that while most features are a pretty straightforward Peripheral feature, those two require the Peripheral device (e.g. the watch) to reach back to the connected Central device (e.g. the phone) to discover and connect to features provided by the Central device.  The existing Qt classes simply do not allow for that possibility, so I have been rewriting replacements that have similar interfaces but do what we need to do.

To that end, the Remote class emulates all of the QLowEnergyController interfaces that make sense for our situation but omits the parts that don't, such as controlling BLE advertisements. By the time we want to look at the services the connected device offers, we're already connected, so by definition, we're beyond the advertising phase.

The last thing to be implemented is an analog to the QLowEnergyController::createServiceObject() function as a Remote::createServiceObject() function.  We can't return a QLowEnergyService object, since there is no public constructor for that class, so we have to come up with an alternative.

Also see https://qt-project.atlassian.net/browse/QTBUG-59925 for a report on this very problem with the Qt implementation.

## Troubleshooting

To aid in troubleshooting, this code can be run and executed on a desktop Linux machine or possibly a Raspberry Pi.  The `CMAKE_CROSSCOMPILING` flag is used to detect and control this case.


