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

#ifndef REMOTEFEATUREREGISTRY_H
#define REMOTEFEATUREREGISTRY_H

#include <functional>
#include <memory>
#include <vector>

#include "remotefeature.h"

// A tiny self-registration mechanism for reverse-direction ("we also act
// like a central") features, mirroring ServiceRegistry (see
// serviceregistry.h) on the peripheral side.
//
// BlueZManager no longer needs to #include ancs.h/cts.h or name an ANCS/CTS
// member: each feature's own .cpp file registers a small factory function
// with this registry (via the REGISTER_REMOTE_FEATURE macro below), and
// BlueZManager just asks the registry to build one instance of every
// registered feature.
//
// Adding a new reverse-direction feature (a future reverse battery-level or
// HRM client, say) therefore only requires: writing the feature's .h/.cpp,
// adding REGISTER_REMOTE_FEATURE(YourFeature) to its .cpp, and listing the
// new files in src/CMakeLists.txt -- no edits to bluezmanager.h/.cpp are
// needed.
class RemoteFeatureRegistry
{
public:
    using Factory = std::function<std::unique_ptr<RemoteFeature>()>;

    static RemoteFeatureRegistry &instance()
    {
        static RemoteFeatureRegistry registry;
        return registry;
    }

    void add(Factory factory)
    {
        mFactories.push_back(std::move(factory));
    }

    // Builds one instance from every registered factory. Called once by
    // BlueZManager's constructor to populate its list of features.
    std::vector<std::unique_ptr<RemoteFeature>> createAll() const
    {
        std::vector<std::unique_ptr<RemoteFeature>> features;
        features.reserve(mFactories.size());
        for (const Factory &factory : mFactories)
            features.push_back(factory());
        return features;
    }

private:
    RemoteFeatureRegistry() = default;
    std::vector<Factory> mFactories;
};

// Registers ClassName with the RemoteFeatureRegistry as a static-
// initialization side effect of loading its translation unit. Invoke this
// once, at namespace scope, in the feature's own .cpp file. ClassName must
// be default-constructible and implement RemoteFeature.
#define REGISTER_REMOTE_FEATURE(ClassName)                                   \
    namespace {                                                              \
    const bool ClassName##_registered = [] {                                 \
        RemoteFeatureRegistry::instance().add(                               \
            [] { return std::make_unique<ClassName>(); });                   \
        return true;                                                         \
    }();                                                                     \
    }

#endif // REMOTEFEATUREREGISTRY_H
