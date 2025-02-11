set(SRC
	main.cpp
        Device.cpp
        Remote.cpp
        BluetoothService.cpp
        HeartRateService.cpp
        BatteryService.cpp
        NotificationService.cpp
        TimeService.cpp
        MediaService.cpp
        ScreenshotService.cpp
        WeatherService.cpp
        ANCSService.cpp
)

set(HEADERS
        Device.h
        Remote.h
        BluetoothService.h
        HeartRateService.h
        BatteryService.h
        NotificationService.h
        TimeService.h
        MediaService.h
        ScreenshotService.h
        WeatherService.h
        ANCSService.h
)

add_executable(asteroid-btsyncd ${SRC} ${HEADERS})
configure_file("asteroid-btsyncd.h.in" "asteroid-btsyncd.h")
target_include_directories(asteroid-btsyncd PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

if (CMAKE_CROSSCOMPILING)
target_link_libraries(asteroid-btsyncd PRIVATE
	Qt5::DBus
        Qt5::Bluetooth
        Qt5::Sensors
	Timed::Timed
	QtMpris::QtMpris
	SystemSettings::SystemSettings
	PkgConfig::GIOMM)

else()
target_link_libraries(asteroid-btsyncd PRIVATE
	Qt5::DBus
        Qt5::Bluetooth
        Qt5::Sensors
        )
endif()



install(TARGETS asteroid-btsyncd
	DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES org.asteroidos.weather.gschema.xml
	DESTINATION ${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas)
