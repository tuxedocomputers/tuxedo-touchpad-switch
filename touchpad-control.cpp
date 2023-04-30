// Copyright (c) 2020 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
//
// This file is part of TUXEDO Touchpad Switch.
//
// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TUXEDO Touchpad Switch is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with TUXEDO Touchpad Switch.  If not, see <https://www.gnu.org/licenses/>.

#include "touchpad-control.h"

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>

#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>

#include <libudev.h>

using std::cout;
using std::cerr;
using std::endl;

// "std::vector<std::string> *devnodes" gets amended with found "/dev/hidraw*" device paths
// returns -EXIT_FAILURE on error or the number of found "i2c-UNIW0001:00"-touchpads, starting with 0, meaning no touchpads found
static int get_touchpad_hidraw_devices(std::vector<std::string> *devnodes) {
    int result = -EXIT_FAILURE;
    
    struct udev *udev_context = udev_new();
    if (!udev_context) {
        cerr << "get_touchpad_hidraw_devices(...): udev_new(...) failed." << endl;
    }
    else {
        struct udev_enumerate *hidraw_devices = udev_enumerate_new(udev_context);
        if (!hidraw_devices) {
            cerr << "get_touchpad_hidraw_devices(...): udev_enumerate_new(...) failed." << endl;
        }
        else {
            if (udev_enumerate_add_match_subsystem(hidraw_devices, "hidraw") < 0) {
                cerr << "get_touchpad_hidraw_devices(...): udev_enumerate_add_match_subsystem(...) failed." << endl;
            }
            else {
                if (udev_enumerate_scan_devices(hidraw_devices) < 0) {
                    cerr << "get_touchpad_hidraw_devices(...): udev_enumerate_scan_devices(...) failed." << endl;
                }
                else {
                    struct udev_list_entry *hidraw_devices_iterator = udev_enumerate_get_list_entry(hidraw_devices);
                    if (!hidraw_devices_iterator) {
                        cerr << "get_touchpad_hidraw_devices(...): udev_enumerate_get_list_entry(...) failed." << endl;
                    }
                    else {
                        struct udev_list_entry *hidraw_device_entry;
                        udev_list_entry_foreach(hidraw_device_entry, hidraw_devices_iterator) {
                            if (strstr(udev_list_entry_get_name(hidraw_device_entry), "i2c-UNIW0001:00")) {
                                struct udev_device *hidraw_device = udev_device_new_from_syspath(udev_context, udev_list_entry_get_name(hidraw_device_entry));
                                if (!hidraw_device) {
                                    cerr << "get_touchpad_hidraw_devices(...): udev_device_new_from_syspath(...) failed." << endl;
                                }
                                else {
                                    std::string devnode = udev_device_get_devnode(hidraw_device);
                                    devnodes->push_back(devnode);
                                    
                                    udev_device_unref(hidraw_device);
                                }
                            }
                        }
                        
                        result = devnodes->size();
                    }
                }
            }
            
            udev_enumerate_unref(hidraw_devices);
        }
        
        udev_unref(udev_context);
    }

    return result;
}

static int get_hidraw_surface_button_switch_report_id(std::string devnode) {
    int result = 0;

    int hidraw = open(devnode.c_str(), O_WRONLY|O_NONBLOCK);
    if (hidraw < 0) {
        cerr << "get_hidraw_surface_button_switch_report_id(...): open(\"" << devnode << "\", O_WRONLY|O_NONBLOCK) failed." << endl;
        return -EXIT_FAILURE;
    }
    else {
        struct hidraw_report_descriptor report_descriptor;

        result = ioctl(hidraw, HIDIOCGRDESCSIZE, &report_descriptor.size);
        if (result < 0) {
            cerr << "get_hidraw_surface_button_switch_report_id(...): ioctl(..., HIDIOCGRDESCSIZE, ...) on " << devnode << " failed." << endl;
            close(hidraw);
            return -EXIT_FAILURE;
        }

        result = ioctl(hidraw, HIDIOCGRDESC, &report_descriptor);
        if (result < 0) {
            cerr << "get_hidraw_surface_button_switch_report_id(...): ioctl(..., HIDIOCGRDESC, ...) on " << devnode << " failed." << endl;
            close(hidraw);
            return -EXIT_FAILURE;
        }

        close(hidraw);

        __u8 surface_button_switch_collection_identifier[] = {0x05, 0x0d, 0x09, 0x22, 0xa1, 0x00, 0x09, 0x57, 0x09, 0x58};
        auto it1 = std::search(std::begin(report_descriptor.value), std::end(report_descriptor.value), std::begin(surface_button_switch_collection_identifier), std::end(surface_button_switch_collection_identifier));
        for (auto it2 = it1; it2 != std::end(report_descriptor.value); ++it2) {
            if (*it2 == 0x85 && std::next(it2) != std::end(report_descriptor.value)) {
                return *(std::next(it2));
            }
        }
    }

    return -EXIT_FAILURE;
}

int set_touchpad_state(int enabled) {
    std::vector<std::string> devnodes;
    int touchpad_count = get_touchpad_hidraw_devices(&devnodes);
    if (touchpad_count < 0) {
        cerr << "send_events_handler(...): get_touchpad_hidraw_devices(...) failed." << endl;
        return EXIT_FAILURE;
    }
    if (touchpad_count == 0) {
        cout << "No compatible touchpads found." << endl;
        return EXIT_FAILURE;
    }
    
    int result = EXIT_SUCCESS;
    
    for (auto it = devnodes.begin(); it != devnodes.end(); ++it) {
        int feature_report_id = get_hidraw_surface_button_switch_report_id(*it);
        if (feature_report_id < 0) {
            cerr << "set_touchpad_state(...): get_hidraw_surface_button_switch_report_id(...) failed." << endl;
            result = EXIT_FAILURE;
        }
        else {
            int hidraw = open((*it).c_str(), O_WRONLY|O_NONBLOCK);
            if (hidraw < 0) {
                cerr << "set_touchpad_state(...): open(\"" << *it << "\", O_WRONLY|O_NONBLOCK) failed." << endl;
                result = EXIT_FAILURE;
            }
            else {
                // To enable touchpad send "0x03" as feature report to the touchpad hid device. The feature report number can be gathered from the report descriptors.
                // To disable it send "0x00".
                // Reference: https://docs.microsoft.com/en-us/windows-hardware/design/component-guidelines/touchpad-configuration-collection#selective-reporting-feature-report
                // Details:
                // The two rightmost bits control the touchpad status
                // In order, they are:
                // 1. LED off + touchpad on/LED on + touchpad off
                // 2. Clicks on/off
                // So, the options are:
                // 0x00 LED on, touchpad off, touchpad click off
                // 0x01 LED on, touchpad off, touchpad click on
                // 0x02 LED off, touchpad on, touchpad click off
                // 0x03 LED off, touchpad on, touchpad click on
                char buffer[2] = {static_cast<char>(feature_report_id), 0x00};
                if (enabled) {
                    buffer[1] = 0x03;
                }

                result = ioctl(hidraw, HIDIOCSFEATURE(sizeof(buffer)/sizeof(buffer[0])), buffer);
                if (result < 0) {
                    cerr << "set_touchpad_state(...): ioctl(...) on " << *it << " failed." << endl;
                    result = EXIT_FAILURE;
                }

                close(hidraw);

                result = EXIT_SUCCESS;
            }
        }
    }
    
    return result;
}

int toggle_touchpad_state() {
    std::vector<std::string> devnodes;
    int touchpad_count = get_touchpad_hidraw_devices(&devnodes);
    if (touchpad_count < 0) {
        cerr << "get_touchpad_hidraw_devices failed." << endl;
        return EXIT_FAILURE;
    }
    if (touchpad_count == 0) {
        cout << "No compatible touchpads found." << endl;
        return EXIT_FAILURE;
    }

    int result = EXIT_SUCCESS;

    for (auto it = devnodes.begin(); it != devnodes.end(); ++it) {
        int hidraw = open((*it).c_str(), O_WRONLY|O_NONBLOCK);
        if (hidraw < 0) {
            cerr << "open(\"" << *it << "\", O_WRONLY|O_NONBLOCK) failed." << endl;
            result = EXIT_FAILURE;
        }
        else {
            // get the device's state first (feature report nr.7 - 0x07)
            char buffer[2] = {0x07, 0x00};
            ioctl(hidraw, HIDIOCGFEATURE(2), buffer);

            // toggle the state
            if (buffer[1] == 0x00) {
                buffer[1] = 0x03; // enable touchpad
            } else {
                buffer[1] = 0x00; // disable touchpad
            }

            int result = ioctl(hidraw, HIDIOCSFEATURE(2), buffer);
            if (result < 0) {
                cerr << "ioctl on " << *it << " failed." << endl;
                result = EXIT_FAILURE;
            }

            close(hidraw);
        }
    }

    return result;
}
