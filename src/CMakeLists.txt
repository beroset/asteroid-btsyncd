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

target_link_libraries(asteroid-btsyncd PRIVATE
	Qt5::DBus
        Qt5::Bluetooth
        Qt5::Sensors
	Timed::Timed
	QtMpris::QtMpris
	SystemSettings::SystemSettings
	PkgConfig::GIOMM)

install(TARGETS asteroid-btsyncd
	DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES org.asteroidos.weather.gschema.xml
	DESTINATION ${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas)
