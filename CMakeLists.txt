# Copyright (c) 2020 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
#
# This file is part of TUXEDO Touchpad Switch.
#
# This file is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# TUXEDO Touchpad Switch is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with TUXEDO Touchpad Switch.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.6)

project(tuxedo-touchpad-switch)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET gio-2.0 libudev)

add_executable(tuxedo-touchpad-switch tuxedo-touchpad-switch.cpp setup-gnome.cpp setup-kde.cpp touchpad-control.cpp)
target_link_libraries(tuxedo-touchpad-switch udev PkgConfig::deps)

add_executable(tuxedo-touchpad-toggle tuxedo-touchpad-toggle.cpp touchpad-control.cpp)
target_link_libraries(tuxedo-touchpad-toggle udev PkgConfig::deps)

install(TARGETS tuxedo-touchpad-switch DESTINATION bin/)
install(TARGETS tuxedo-touchpad-toggle DESTINATION bin/)
install(FILES res/99-tuxedo-touchpad-switch.rules DESTINATION lib/udev/rules.d/)
install(FILES res/tuxedo-touchpad-switch-lockfile DESTINATION /etc/ PERMISSIONS OWNER_READ GROUP_READ WORLD_READ) # absolute path on purpose: implemented as such in tuxedo-touchpad-switch.cpp
install(FILES res/tuxedo-touchpad-switch.desktop DESTINATION /usr/share/gdm/greeter/autostart/) # absolute path on purpose: gdm has no config dir in /usr/local/
install(FILES res/tuxedo-touchpad-switch.desktop DESTINATION /etc/xdg/autostart/) # absolute path on purpose: $XDG_CONFIG_DIRS does not include a folder under /usr/ by default https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html#variables
