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

set(
    HID_KEYBOARD_SRC
    hidkeyboardmain.cpp
    hidkeyboardapplication.cpp
    hidkeyboardservice.cpp
    keyboardinput.cpp
    service.cpp
    characteristic.cpp
    notifyingcharacteristic.cpp
    descriptor.cpp
    bluezmanager.cpp
    advertisement.cpp
    remote/bluezobjects.cpp
)
set(
    HID_KEYBOARD_HEADERS
    hidkeyboardapplication.h
    hidkeyboardservice.h
    keyboardreports.h
    keyboardinput.h
    service.h
    characteristic.h
    notifyingcharacteristic.h
    descriptor.h
    bluezmanager.h
    advertisement.h
    common.h
    remote/bluezobjects.h
    remote/remotefeature.h
    remote/remotefeatureregistry.h
)

add_executable(asteroid-hid-keyboard ${HID_KEYBOARD_SRC} ${HID_KEYBOARD_HEADERS})
target_include_directories(asteroid-hid-keyboard PRIVATE remote)
target_compile_features(asteroid-hid-keyboard PRIVATE cxx_std_17)
set_target_properties(asteroid-hid-keyboard PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
target_link_libraries(asteroid-hid-keyboard PRIVATE Qt6::Core Qt6::DBus)

install(TARGETS asteroid-hid-keyboard DESTINATION ${CMAKE_INSTALL_BINDIR})
configure_file(
    asteroid-hid-keyboard.service
    ${CMAKE_CURRENT_BINARY_DIR}/asteroid-hid-keyboard.service
    @ONLY
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/asteroid-hid-keyboard.service
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/system
)
install(
    FILES org.asteroidos.HidKeyboard.conf
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system.d
)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(
        keyboardreports-test
        tests/keyboardreports_test.cpp
        hidkeyboardservice.cpp
        service.cpp
        characteristic.cpp
        notifyingcharacteristic.cpp
        descriptor.cpp
        hidkeyboardservice.h
        service.h
        characteristic.h
        notifyingcharacteristic.h
        descriptor.h
    )
    target_include_directories(keyboardreports-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(keyboardreports-test PRIVATE Qt6::DBus Qt6::Test)
    set_target_properties(keyboardreports-test PROPERTIES AUTOMOC ON)
    target_compile_features(keyboardreports-test PRIVATE cxx_std_17)
    add_test(NAME keyboardreports COMMAND keyboardreports-test)
endif()
