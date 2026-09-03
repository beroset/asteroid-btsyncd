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

#ifndef SERVICEREGISTRY_H
#define SERVICEREGISTRY_H

#include <QDBusConnection>
#include <functional>
#include <vector>

class Service;

// A tiny self-registration mechanism for peripheral GATT services.
//
// Application no longer needs to #include and new() every Service subclass
// by hand: each service's own .cpp file registers a small factory function
// with this registry (via the REGISTER_SERVICE macro below), and
// Application just asks the registry to build one instance of every
// registered service, in whatever order they happened to register.
//
// That order is unspecified across translation units, but it is also
// harmless here: the "index" passed to a Service's constructor is only
// ever used to build a unique D-Bus object path
// (SERVICE_PATH_BASE + QString::number(index)) and carries no other
// meaning, so any assignment of distinct indices is fine (see service.cpp).
//
// Adding a new peripheral service therefore only requires: writing the
// service's .h/.cpp, adding REGISTER_SERVICE(YourService) to its .cpp, and
// listing the new files in src/CMakeLists.txt -- no edits to
// application.h/.cpp are needed.
class ServiceRegistry
{
public:
    using Factory = std::function<Service *(QDBusConnection, int)>;

    static ServiceRegistry &instance()
    {
        static ServiceRegistry registry;
        return registry;
    }

    void add(Factory factory)
    {
        mFactories.push_back(std::move(factory));
    }

    const std::vector<Factory> &factories() const
    {
        return mFactories;
    }

private:
    ServiceRegistry() = default;
    std::vector<Factory> mFactories;
};

// Registers ClassName with the ServiceRegistry as a static-initialization
// side effect of loading its translation unit. Invoke this once, at
// namespace scope, in the service's own .cpp file.
#define REGISTER_SERVICE(ClassName)                                          \
    namespace {                                                              \
    const bool ClassName##_registered = [] {                                 \
        ServiceRegistry::instance().add([](QDBusConnection bus, int index) { \
            return new ClassName(index, bus);                                \
        });                                                                  \
        return true;                                                         \
    }();                                                                     \
    }

#endif // SERVICEREGISTRY_H
