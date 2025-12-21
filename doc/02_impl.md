# Implementation notes {#implementation} #

This page contains notes about how this implementation was created, which includes some hints about debugging and testing.

## Overview

The `asteroid-btsyncd` software interfaces via D-Bus with an underlying BlueZ instance and acts as a Peripheral device.  This is supported well enough by the Qt classes, but in the case of ANCS and some other possible new facilities, the Qt classes do not support Bluetooth completely.  Working around this was the reason for the introduction of the Remote class, but that is not yet completely implemented.

## Troubleshooting

To aid in troubleshooting, this code can be run and executed on a desktop Linux machine or possibly a Raspberry Pi.  The `CMAKE_CROSSCOMPILING` flag is used to detect and control this case.


