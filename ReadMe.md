This a proof-of-concept Linux userspace driver to hardware-enable/-disable the touchpads on TongFang/Uniwill laptops. This will also trigger the disabled-led, formerly unfunctional under linux.

It syncs up with the software toggle in the gnome settings deamon.

It comes with some known disfuctionalities:
    - only works on desktop evnironments using gsettings/gio
    - does not work on loginscreen(gdm)
    - needs to be started for every user seperatly
    - no script/package to setup autostart
    - every user needs write access to the /dev/hidraw* device for the touchpad (udev *.rules giving read and write access included, could be trimmed down to write only)
    - switching between 2 users is not detected (on next toggle the software and hardware disable will be in sync again however)

Author: Werner Sembach

Raw i2c communication with the touchpad for reference:
    
```
0x22 0x00 0x37 0x03 0x23 0x00 0x04 0x00 0x07 0x03
 [^^   ^^] (?max?) length of report? in little endian
            ^ 2 bits reserved + report type
             ^ report id (defined by touchpad controller?
                 ^ reserved
                  ^ opcode (0x2 = GET_REPORT, 0x3 = SET_REPORT)
                     [^^   ^^   ^^   ^^   ^^] ?i2c-hid stuff?
                                           ^ ?report id again?
                                               ^^ report/payload: defined by touchpad controller? 0x03 enabled (0x02 auch, keine ahnung was das unterste bit tut), 0x00 disabled
```
