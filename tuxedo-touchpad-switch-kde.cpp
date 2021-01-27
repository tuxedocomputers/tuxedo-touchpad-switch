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

#include <linux/hidraw.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <signal.h>

#include <libudev.h>

#include <gio/gio.h>

#include <iostream>
#include <string>
#include <vector>

using std::cout;
using std::cerr;
using std::endl;

int lockfile;
gboolean isMousePluggedInPrev;
gboolean isEnabledSave;

static int get_touchpad_hidraw_devices(std::vector<std::string> *devnodes) {
    int result = -EXIT_FAILURE;
    
    struct udev *udev_context = udev_new();
    if (!udev_context) {
        cerr << "get_touchpad_hidraw_devices(...): udev_new(...) failed." << endl;
        return result;
    }

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

    return result;
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
    
    for (auto it = devnodes.begin(); it != devnodes.end(); ++it) {
        int hidraw = open((*it).c_str(), O_WRONLY|O_NONBLOCK);
        if (hidraw < 0) {
            cerr << "send_events_handler(...): open(\"" << *it << "\", O_WRONLY|O_NONBLOCK) failed." << endl;
            continue;
        }

        // default is to enable touchpad, for this send "0x03" as feature report nr.7 (0x07) to the touchpad hid device
        char buffer[2] = {0x07, 0x00};
        // change 0x03 to 0x00 to disable touchpad when configuration string starts with [d]isable
        if (enabled) {
            buffer[1] = 0x03;
        }
        int result = ioctl(hidraw, HIDIOCSFEATURE(sizeof(buffer)/sizeof(buffer[0])), buffer);
        if (result < 0) {
            cerr << "send_events_handler(...): ioctl(...) on " << *it << " failed." << endl;
            close(hidraw);
            continue;
        }

        close(hidraw);
    }
    
    return EXIT_SUCCESS;
}

void gracefull_exit(int signum) {
    int result = set_touchpad_state(1);
    if (flock(lockfile, LOCK_UN)) {
        cerr << "main(...): flock(...) failed." << endl;
    }
    close(lockfile);
    exit(result);
}

void  kded5_modules_touchpad_handler(GDBusProxy *proxy, __attribute__((unused)) char *sender_name, char *signal_name, GVariant *parameters, __attribute__((unused)) gpointer user_data) {
    if (!strcmp("enabledChanged", signal_name) && g_variant_is_of_type(parameters, (const GVariantType *)"(b)") && g_variant_n_children(parameters)) {
        GVariant *enabledChanged = g_variant_get_child_value(parameters, 0);
        
        isEnabledSave = g_variant_get_boolean(enabledChanged);
        if (isEnabledSave) {
            if (set_touchpad_state(1)) {
                cerr << "kded5_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
            }
        }
        else {
            if (set_touchpad_state(0)) {
                cerr << "kded5_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
            }
        }
        g_variant_unref(enabledChanged);
    }
    else if (!strcmp("mousePluggedInChanged", signal_name) && g_variant_is_of_type(parameters, (const GVariantType *)"(b)") && g_variant_n_children(parameters)) {
        GVariant *isMousePluggedInParam = g_dbus_proxy_call_sync(proxy, "isMousePluggedIn", NULL, G_DBUS_CALL_FLAGS_NONE, G_MAXINT, NULL, NULL);
        if (isMousePluggedInParam != NULL && g_variant_is_of_type(isMousePluggedInParam, (const GVariantType *)"(b)") && g_variant_n_children(isMousePluggedInParam)) {
            GVariant *isMousePluggedIn = g_variant_get_child_value(isMousePluggedInParam, 0);
            if (!g_variant_get_boolean(isMousePluggedIn)) {
                if (set_touchpad_state(1)) {
                    cerr << "kded5_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
                }
                if (flock(lockfile, LOCK_UN)) {
                    cerr << "kded5_modules_touchpad_handler(...): flock(...) failed." << endl;
                }
            }
            else if (!isMousePluggedInPrev) {
                if (flock(lockfile, LOCK_EX)) {
                    cerr << "kded5_modules_touchpad_handler(...): flock(...) failed." << endl;
                }
                if (set_touchpad_state(isEnabledSave)) {
                    cerr << "kded5_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
                }
            }
            isMousePluggedInPrev = g_variant_get_boolean(isMousePluggedIn);
            g_variant_unref(isMousePluggedIn);
            g_variant_unref(isMousePluggedInParam);
        }
        else {
            cerr << "kded5_modules_touchpad_handler(...): g_dbus_proxy_call_sync(...) failed." << endl;
        }
    }
}

int kded5_modules_touchpad_init(GDBusProxy *proxy) {
    GVariant *isEnabledParam = g_dbus_proxy_call_sync(proxy, "isEnabled", NULL, G_DBUS_CALL_FLAGS_NONE, G_MAXINT, NULL, NULL);
    if (isEnabledParam != NULL && g_variant_is_of_type(isEnabledParam, (const GVariantType *)"(b)") && g_variant_n_children(isEnabledParam)) {
        GVariant *isEnabled = g_variant_get_child_value(isEnabledParam, 0);

        // init isEnabledSave
        isEnabledSave = g_variant_get_boolean(isEnabled);
        if (isEnabledSave) {
            if (set_touchpad_state(1)) {
                cerr << "kded5_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
                return EXIT_FAILURE;
            }
        }
        else {
            if (set_touchpad_state(0)) {
                cerr << "kded5_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
                return EXIT_FAILURE;
            }
        }

        g_variant_unref(isEnabled);
        g_variant_unref(isEnabledParam);
    }
    else {
        cerr << "kded5_modules_touchpad_handler(...): g_dbus_proxy_call_sync(...) failed." << endl;
        return EXIT_FAILURE;
    }

    GVariant *isMousePluggedInParam = g_dbus_proxy_call_sync(proxy, "isMousePluggedIn", NULL, G_DBUS_CALL_FLAGS_NONE, G_MAXINT, NULL, NULL);
    if (isMousePluggedInParam != NULL && g_variant_is_of_type(isMousePluggedInParam, (const GVariantType *)"(b)") && g_variant_n_children(isMousePluggedInParam)) {
        GVariant *isMousePluggedIn = g_variant_get_child_value(isMousePluggedInParam, 0);
        
        // init isMousePluggedInPrev, should always be true on init, this is here to be on the save side
        isMousePluggedInPrev = g_variant_get_boolean(isMousePluggedIn);
        
        // isMousePluggedInPrev just got init so it holds the current value
        if (!isMousePluggedInPrev) {
            if (set_touchpad_state(1)) {
                cerr << "kded5_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
                return EXIT_FAILURE;
            }
            if (flock(lockfile, LOCK_UN)) {
                cerr << "kded5_modules_touchpad_handler(...): flock(...) failed." << endl;
                return EXIT_FAILURE;
            }
        }
        
        g_variant_unref(isMousePluggedIn);
        g_variant_unref(isMousePluggedInParam);
    }
    else {
        cerr << "kded5_modules_touchpad_handler(...): g_dbus_proxy_call_sync(...) failed." << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main() {
    lockfile = open("/etc/tuxedo-touchpad-switch-lockfile", O_RDONLY);
    if (lockfile < 0) {
        cerr << "main(...): open(...) failed." << endl;
        gracefull_exit(0);
    }
    
    //FIXME singal(...) deprecated -> man signal
    signal(SIGINT, gracefull_exit);
    signal(SIGTERM, gracefull_exit);
    
    if (flock(lockfile, LOCK_EX)) {
        cerr << "main(...): flock(...) failed." << endl;
        gracefull_exit(0);
    }


    // sync on config change, xsession change, and wakeup kde
    GDBusProxy *kded5_modules_touchpad = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                                       G_DBUS_PROXY_FLAGS_NONE, NULL,
                                                                       "org.kde.kded5",
                                                                       "/modules/touchpad",
                                                                       "org.kde.touchpad",
                                                                       NULL, NULL);
    if (kded5_modules_touchpad == NULL) {
        cerr << "main(...): g_dbus_proxy_new_for_bus_sync(...) failed." << endl;
        gracefull_exit(0);
    }
    if (g_signal_connect(kded5_modules_touchpad, "g-signal", G_CALLBACK(kded5_modules_touchpad_handler), NULL) < 1) {
        cerr << "main(...): g_signal_connect(...) failed." << endl;
        gracefull_exit(0);
    }
    
    
    // sync on start
    if (kded5_modules_touchpad_init(kded5_modules_touchpad) == EXIT_FAILURE) {
        cerr << "main(...): kded5_modules_touchpad_init(...) failed." << endl;
        gracefull_exit(0);
    }
    
    
    // start empty glib mainloop, required for glib signals to be catched
    GMainLoop *app = g_main_loop_new(NULL, TRUE);
    if (!app) {
        cerr << "main(...): g_main_loop_new(...) failed." << endl;
        gracefull_exit(0);
    }
    
    g_main_loop_run(app);
    // g_main_loop_run only returns on error
    cerr << "main(...): g_main_loop_run(...) failed." << endl;
    gracefull_exit(0);
}
