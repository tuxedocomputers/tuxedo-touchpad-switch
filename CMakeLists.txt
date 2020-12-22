cmake_minimum_required(VERSION 3.6)

project(tuxedo-touchpad-switch)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET gio-2.0 udev)

add_executable(tuxedo-touchpad-switch tuxedo-touchpad-switch.cpp)
target_link_libraries(tuxedo-touchpad-switch udev PkgConfig::deps)

install(TARGETS tuxedo-touchpad-switch DESTINATION bin/)
install(FILES res/99-tuxedo-touchpad-switch.rules DESTINATION lib/udev/rules.d/)
install(FILES res/tuxedo-touchpad-switch.desktop DESTINATION /etc/xdg/autostart/) # absolute path on purpose for this file only:
                                                                                  # $XDG_CONFIG_DIRS dos not include a folder under /usr/ by default 
                                                                                  # https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html#variables

SET(CPACK_GENERATOR DEB RPM)
SET(CPACK_PACKAGE_CONTACT "Werner Sembach <tux@tuxedocomputers.com>")
SET(CPACK_PACKAGE_VERSION 0.0.1)
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hardware toggle for Tongfang/Uniwill i2c touchpads. This will toggle the touchpad-disabled-led.")
SET(CPACK_DEBIAN_PACKAGE_SECTION admin)
include(CPack)
