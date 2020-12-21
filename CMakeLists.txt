cmake_minimum_required(VERSION 3.6)

project(tuxedo-touchpad-switch)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET gio-2.0 udev)

add_executable(tuxedo-touchpad-switch tuxedo-touchpad-switch.cpp)
target_link_libraries(tuxedo-touchpad-switch udev PkgConfig::deps)
