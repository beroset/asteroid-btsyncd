cmake_minimum_required(VERSION 3.6.0)

project(asteroid-btsyncd
	VERSION 2.0.0
	DESCRIPTION "Bluetooth Low Energy synchronization daemon for AsteroidOS")

find_package(ECM REQUIRED NO_MODULE)
if (CMAKE_CROSSCOMPILING)
    find_package(AsteroidApp REQUIRED)
else()
endif()

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ASTEROID_MODULE_PATH})

include(FeatureSummary)
include(GNUInstallDirs)
include(ECMFindQmlModule)
if (CMAKE_CROSSCOMPILING)
    include(AsteroidCMakeSettings)
    include(AsteroidTranslations)
else()
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)
    set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
endif()

find_package(Qt5 COMPONENTS DBus Bluetooth Sensors REQUIRED)
if (CMAKE_CROSSCOMPILING)
    find_package(Timed REQUIRED)
    find_package(QtMpris REQUIRED)
    find_package(SystemSettings REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GIOMM giomm-2.4 REQUIRED IMPORTED_TARGET)
set_property(GLOBAL APPEND PROPERTY _CMAKE_giomm-2.4_TYPE REQUIRED)

if(GIOMM_FOUND)
	set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND giomm-2.4)
else()
	set_property(GLOBAL APPEND PROPERTY PACKAGES_NOT_FOUND giomm-2.4)
endif()

else()
add_subdirectory(doc)
endif()

add_subdirectory(src)

if (CMAKE_CROSSCOMPILING)
build_translations(i18n)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
