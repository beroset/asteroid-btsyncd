set(SRC
	main.cpp
        Device.cpp
        BluetoothService.cpp
        HeartRateService.cpp
        BatteryService.cpp
        NotificationService.cpp
)

set(HEADERS
        Device.h
        BluetoothService.h
        HeartRateService.h
        BatteryService.h
        NotificationService.h
)

add_executable(asteroid-btsyncd ${SRC} ${HEADERS})

target_link_libraries(asteroid-btsyncd PRIVATE
	Qt5::DBus
        Qt5::Bluetooth
	Timed::Timed
	QtMpris::QtMpris
	SystemSettings::SystemSettings
	PkgConfig::GIOMM)
	

install(TARGETS asteroid-btsyncd
	DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES org.asteroidos.weather.gschema.xml
	DESTINATION ${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas)
