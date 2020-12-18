cmake_minimum_required(VERSION 3.6)

project(tuxedo-touchpad-switch)

find_package(PkgConfig REQUIRED)

add_executable(tuxedo-touchpad-switch tuxedo-touchpad-switch.cpp)
target_link_libraries(tuxedo-touchpad-switch udev)

pkg_check_modules(GIO2 REQUIRED IMPORTED_TARGET gio-2.0)

add_executable(gsettings-test gsettings-test.c)
target_link_libraries(gsettings-test PkgConfig::GIO2)

add_executable(tuxedo-touchpad-switch-deamon tuxedo-touchpad-switch-deamon.cpp)
target_link_libraries(tuxedo-touchpad-switch-deamon udev PkgConfig::GIO2)
