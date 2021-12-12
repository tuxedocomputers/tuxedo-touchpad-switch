# About
This a Linux daemon or standalone program to enable and disable the touchpads on TongFang/Uniwill laptops using a HID command. **This will trigger the touchpad-disabled-LED, formerly unfunctional under linux.**

Most desktop environments already have a way to disable the touchpad, but this setting never reaches the firmware of the device. This daemon listens to the session D-Bus and dispatches the approprita HID call to /dev/hidraw* whenever the setting changes, closing the gap to the device itself, and enabling the built in LED.

This program has two executables:

1. `tuxedo-touchpad-switch-daemon`: The daemon executable. Reacts to touchpad system events. Must be run as root.
2. `tuxedo-touchpad-switch-cli`: The command line executable. Must be run as root.

## Daemon
The daemon executable is tested and works on the GDM greeter, GNOME Shell, Budgie, and KDE Plasmashell.  
For all other environments, including the tty-console, only the cli executable is available.

## CLI
The cli executable is available as an alternative to the daemon, in case the system is not supported or in case you are a power user who prefers to use it instead.

The cli has options to toggle and options to set the touchpad on/off status.


## Author
Author: Werner Sembach <tux@tuxedocomputers.com>


# Install/Use:

## Common steps
```
$ sudo apt install libudev-dev libglib2.0-dev
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Installing
```
$ sudo make install
```

To use automatically as a daemon, after installing via `make install` or using the .deb, you need to reboot your system for the daemon to load.

Otherwise, a reboot isn't necessary. 

## Using

Note: While there's an instance running as a daemon, the other running modes become unusable.

### Daemon

Start the daemon by running:  
`sudo tuxedo-touchpad-switch-daemon`


### CLI
#### Toggle touchpad

You may toggle between touchpad on and off like so
`sudo tuxedo-touchpad-switch-cli -t`
The new toggled state will be outputted to stdout.


#### Get state

You may get the touchpad hardware state.
`sudo tuxedo-touchpad-switch-cli -g`
The current state will be returned


#### Set state

You may set the touchpad hardware state.
`sudo tuxedo-touchpad-switch-cli -s <state>`
The options are:
1. on → Touchpad is on, LED is OFF
2. touch → Touchpad is on only for touching. Clicking is off, LED is OFF
3. off → Touchpad is off, LED is ON

#### Instructions
For the included instructions, run `tuxedo-touchpad-switch-cli --help`

# Development

## Testing installation

```
$ sudo make install
```
`tuxedo-touchpad-switch-cli` executable should now be available as a command.

There is also a target `make package` which is fine for testing, but it will not create a debian best practices compliant .deb. For this you need to use `gbp buildpackage` as described below.

## Packaging
```
$ sudo apt install libudev-dev libglib2.0-dev git-buildpackage debhelper
$ rm -r build
$ gbp buildpackage -uc -us
```
A .deb package is created in the folder above the git repository.

