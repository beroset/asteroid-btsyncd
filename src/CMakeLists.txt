set(
    SERVICES
    notificationservice.cpp
    weatherservice.cpp
    mediaservice.cpp
    batteryservice.cpp
    screenshotservice.cpp
    timeservice.cpp
)
set(
    REMOTESERVICES
    remote/ancs.cpp
    remote/cts.cpp
)
set(
    SRC
    ${SERVICES}
    main.cpp
    batterystatus.cpp
    settime.cpp
    service.cpp
    characteristic.cpp
    notifyingcharacteristic.cpp
    descriptor.cpp
    application.cpp
    advertisement.cpp
    bluezmanager.cpp
    ${REMOTESERVICES}
    remote/bluezobjects.cpp
    remote/remotecharacteristic.cpp
    remote/ancs_notification.cpp
)
set(
    HEADERS
    notificationservice.h
    weatherservice.h
    mediaservice.h
    batteryservice.h
    batterystatus.h
    screenshotservice.h
    settime.h
    timeservice.h
    service.h
    characteristic.h
    notifyingcharacteristic.h
    descriptor.h
    common.h
    application.h
    serviceregistry.h
    advertisement.h
    bluezmanager.h
    remote/bluezobjects.h
    remote/remotecharacteristic.h
    remote/remotefeature.h
    remote/remotefeatureregistry.h
    remote/gattbytes.h
    remote/ancs_protocol_constants.h
    remote/ancs.h
    remote/ancs_notification.h
    remote/cts.h
)

add_executable(asteroid-btsyncd ${SRC} ${HEADERS})
target_include_directories(asteroid-btsyncd PRIVATE remote)
target_compile_features(asteroid-btsyncd PRIVATE cxx_std_23)
set_target_properties(asteroid-btsyncd PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(
    asteroid-btsyncd
    PRIVATE
        Qt6::Core
        Qt6::DBus
        PkgConfig::AmberMpris
        PkgConfig::MLITE
)

install(
    TARGETS asteroid-btsyncd
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
