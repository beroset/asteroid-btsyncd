/*
 * Copyright (C) 2026 - The asteroid-btsyncd contributors
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

#ifndef REMOTEFEATURE_H
#define REMOTEFEATURE_H

// Common interface implemented by every "reach back to the central device"
// feature built on top of RemoteCharacteristic (see remotecharacteristic.h
// for why this reverse-GATT-client layer exists at all).
//
// BlueZManager keeps a plain list of RemoteFeature pointers and drives every
// one of them the same way from onServicesResolvedChanged()/
// onConnectedChanged(), instead of separately hand-calling each feature's
// own differently-named search/disconnect methods. Adding a new
// reverse-direction feature (a future reverse battery-level or HRM client,
// say) means implementing this interface and adding one instance to that
// list in BlueZManager's constructor -- no other wiring changes are needed.
class RemoteFeature
{
public:
    virtual ~RemoteFeature() = default;

    // Called once BlueZ reports the paired device's GATT services are
    // resolved: locate and subscribe to whatever characteristic(s) this
    // feature depends on.
    virtual void search() = 0;

    // Called when the central disconnects: drop any subscriptions/state so
    // a later reconnect starts from a clean slate.
    virtual void disconnect() = 0;
};

#endif // REMOTEFEATURE_H
