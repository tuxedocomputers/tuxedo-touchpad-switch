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

void send_events_handler(GSettings *settings, const char* key, __attribute__((unused)) gpointer user_data) {
    const gchar *send_events_string = g_settings_get_string(settings, key);
    if (!send_events_string) {
        cerr << "send_events_handler(...): g_settings_get_string(...) failed." << endl;
        return;
    }
    
    int enabled = 0;
    if (send_events_string[0] == 'e') {
        enabled = 1;
    }
    
    if (set_touchpad_state(enabled)) {
        cerr << "send_events_handler(...): set_touchpad_state(...) failed." << endl;
    }
}

void  properties_changed_handler(__attribute__((unused)) GDBusProxy *proxy, GVariant *changed_properties, __attribute__((unused)) GStrv invalidated_properties, __attribute__((unused)) gpointer user_data) {
    if (g_variant_is_of_type(changed_properties, G_VARIANT_TYPE_VARDICT)) {
        GVariantDict changed_properties_dict;
        gboolean sessionIsActive;

        g_variant_dict_init (&changed_properties_dict, changed_properties);
        if (g_variant_dict_lookup (&changed_properties_dict, "SessionIsActive", "b", &sessionIsActive)) {
            if (sessionIsActive) {
                if (flock(lockfile, LOCK_EX)) {
                    cerr << "main(...): flock(...) failed." << endl;
                }
                send_events_handler((GSettings *)user_data, "send-events", NULL);
            }
            else {
                if (set_touchpad_state(1)) {
                    cerr << "properties_changed_handler(...): set_touchpad_state(...) failed." << endl;
                }
                if (flock(lockfile, LOCK_UN)) {
                    cerr << "main(...): flock(...) failed." << endl;
                }
            }
        }
    }
}

int main() {
    // Currently this programm works on desktop environments using Gnome settings only (Gnome/Budgie/Cinnamon/etc.).
    // A KDE configuration version will be developt once this works flawless.
    lockfile = open("/etc/tuxedo-touchpad-switch-lockfile", O_RDONLY);
    if (lockfile < 0) {
        cerr << "main(...): open(...) failed." << endl;
        return EXIT_FAILURE;
    }
    
    signal(SIGINT, gracefull_exit);
    signal(SIGTERM, gracefull_exit);
    
    if (flock(lockfile, LOCK_EX)) {
        cerr << "main(...): flock(...) failed." << endl;
        close(lockfile);
        return EXIT_FAILURE;
    }
    
    // get a new glib settings context to read the touchpad configuration of the current user
    GSettings *touchpad_settings = g_settings_new("org.gnome.desktop.peripherals.touchpad");
    if (!touchpad_settings) {
        cerr << "main(...): g_settings_new(...) failed." << endl;
        close(lockfile);
        return EXIT_FAILURE;
    }
    
    
    // sync on config change
    if (g_signal_connect(touchpad_settings, "changed::send-events", G_CALLBACK(send_events_handler), NULL) < 1) {
        cerr << "main(...): g_signal_connect(...) failed." << endl;
        close(lockfile);
        return EXIT_FAILURE;
    }
    
    
    // sync on start
    send_events_handler(touchpad_settings, "send-events", NULL);
    
    
    // sync on xsession change
    GDBusProxy *session_manager_properties = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, NULL, "org.gnome.SessionManager", "/org/gnome/SessionManager", "org.gnome.SessionManager", NULL, NULL);
    if (session_manager_properties == NULL) {
        cerr << "main(...): g_dbus_proxy_new_for_bus_sync(...) failed." << endl;
        close(lockfile);
        return EXIT_FAILURE;
    }
    
    if (g_signal_connect(session_manager_properties, "g-properties-changed", G_CALLBACK(properties_changed_handler), touchpad_settings) < 1) {
        cerr << "main(...): g_signal_connect(...) failed." << endl;
        close(lockfile);
        g_object_unref(session_manager_properties);
        return EXIT_FAILURE;
    }
    
    
    // start empty glib mainloop, required for glib signals to be catched
    GMainLoop *app = g_main_loop_new(NULL, TRUE);
    if (!app) {
        cerr << "main(...): g_main_loop_new(...) failed." << endl;
        close(lockfile);
        g_object_unref(session_manager_properties);
        return EXIT_FAILURE;
    }
    
    g_main_loop_run(app);
    // g_main_loop_run should not return
    cerr << "main(...): g_main_loop_run(...) failed." << endl;
    close(lockfile);
    g_object_unref(session_manager_properties);
    return EXIT_FAILURE;
}
