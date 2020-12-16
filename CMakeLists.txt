project(tuxedo-touchpad-switch)
add_executable(tuxedo-touchpad-switch tuxedo-touchpad-switch.cpp)
target_link_libraries(tuxedo-touchpad-switch udev)
