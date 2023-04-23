# About
This is a Linux userspace driver to enable and disable the touchpads on TongFang/Uniwill laptops using a HID command. **This will trigger the touchpad-disabled-LED, formerly unfunctional under linux.**

Most desktop environments already have a way to disable the touchpad, but this setting never reaches the firmware of the device. This driver listens to the session D-Bus and dispatches the appropriate HID call to /dev/hidraw* whenever the setting changes, closing the gap to the device itself, and enabling the built in LED.

Currently this driver was only tested and works on the GDM greeter, GNOME Shell, Budgie, and KDE Plasmashell. All other environments, including the tty-console, work as before, meaning touchpad is always enabled on the HID level.

Author: Werner Sembach <tux@tuxedocomputers.com>

# Building

## Testing
```
$ sudo apt install libudev-dev libglib2.0-dev
$ mkdir build
$ cd build
$ cmake ..
$ sudo make install
$ sudo reboot
```

## Packaging
```
$ sudo apt install libudev-dev libglib2.0-dev git-buildpackage debhelper
$ rm -r build
$ gbp buildpackage -uc -us
```
A .deb package is created in the folder above the git repository.

# Installing

After installing via `make install` or using the .deb you need to reboot your system for the driver to load.
