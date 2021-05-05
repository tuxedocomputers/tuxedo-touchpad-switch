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

#include "setup-gnome.h"

#include <iostream>

#include <sys/file.h>

#include <gio/gio.h>

#include "touchpad-control.h"

using std::cerr;
using std::endl;

static int lockfile;
static GSettings *touchpad_settings = NULL;
static GDBusProxy *session_manager_properties = NULL;
static GDBusProxy *display_config_properties = NULL;

static void send_events_handler(GSettings *settings, const char* key, __attribute__((unused)) gpointer user_data) {
    const gchar *send_events_string = g_settings_get_string(settings, key);
    if (!send_events_string) {
        cerr << "send_events_handler(...): g_settings_get_string(...) failed." << endl;
        return;
    }
    
    int enabled = TOUCHPAD_DISABLE;
    if (send_events_string[0] == 'e') {
        enabled = TOUCHPAD_ENABLE;
    }
    
    if (set_touchpad_state(enabled)) {
        cerr << "send_events_handler(...): set_touchpad_state(...) failed." << endl;
    }
}

static void  session_manager_properties_changed_handler(__attribute__((unused)) GDBusProxy *proxy, GVariant *changed_properties, __attribute__((unused)) GStrv invalidated_properties, gpointer user_data) {
    if (g_variant_is_of_type(changed_properties, G_VARIANT_TYPE_VARDICT)) {
        GVariantDict changed_properties_dict;
        gboolean sessionIsActive;

        g_variant_dict_init (&changed_properties_dict, changed_properties);
        if (g_variant_dict_lookup (&changed_properties_dict, "SessionIsActive", "b", &sessionIsActive)) {
            if (sessionIsActive) {
                if (flock(lockfile, LOCK_EX)) {
                    cerr << "properties_changed_handler(...): flock(...) failed." << endl;
                }
                send_events_handler((GSettings *)user_data, "send-events", NULL);
            }
            else {
                if (set_touchpad_state(TOUCHPAD_ENABLE)) {
                    cerr << "properties_changed_handler(...): set_touchpad_state(...) failed." << endl;
                }
                if (flock(lockfile, LOCK_UN)) {
                    cerr << "properties_changed_handler(...): flock(...) failed." << endl;
                }
            }
        }
    }
}

static void  display_config_properties_changed_handler(__attribute__((unused)) GDBusProxy *proxy, GVariant *changed_properties, __attribute__((unused)) GStrv invalidated_properties, gpointer user_data) {
    if (g_variant_is_of_type(changed_properties, G_VARIANT_TYPE_VARDICT)) {
        GVariantDict changed_properties_dict;
        gint32 powerSaveMode;

        g_variant_dict_init (&changed_properties_dict, changed_properties);
        if (g_variant_dict_lookup (&changed_properties_dict, "PowerSaveMode", "i", &powerSaveMode)) {
            if (powerSaveMode == 0) {
                send_events_handler((GSettings *)user_data, "send-events", NULL);
            }
        }
    }
}

int setup_gnome(int lockfile_arg) {
    lockfile = lockfile_arg;
    
    // get a new glib settings context to read the touchpad configuration of the current user
    touchpad_settings = g_settings_new("org.gnome.desktop.peripherals.touchpad");
    if (!touchpad_settings) {
        cerr << "main(...): g_settings_new(...) failed." << endl;
        return EXIT_FAILURE;
    }
    
    // sync on config change
    if (g_signal_connect(touchpad_settings, "changed::send-events", G_CALLBACK(send_events_handler), NULL) < 1) {
        cerr << "main(...): g_signal_connect(...) failed." << endl;
        return EXIT_FAILURE;
    }
    
    // sync on xsession change
    GDBusProxy *session_manager_properties = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                                           G_DBUS_PROXY_FLAGS_NONE, NULL,
                                                                           "org.gnome.SessionManager",
                                                                           "/org/gnome/SessionManager",
                                                                           "org.gnome.SessionManager",
                                                                           NULL, NULL);
    if (session_manager_properties == NULL) {
        cerr << "main(...): g_dbus_proxy_new_for_bus_sync(...) failed." << endl;
        return EXIT_FAILURE;
    }
    if (g_signal_connect(session_manager_properties, "g-properties-changed", G_CALLBACK(session_manager_properties_changed_handler), touchpad_settings) < 1) {
        cerr << "main(...): g_signal_connect(...) failed." << endl;
        return EXIT_FAILURE;
    }
    
    // sync on wakeup
    GDBusProxy *display_config_properties = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                                          G_DBUS_PROXY_FLAGS_NONE, NULL,
                                                                          "org.gnome.Mutter.DisplayConfig",
                                                                          "/org/gnome/Mutter/DisplayConfig",
                                                                          "org.gnome.Mutter.DisplayConfig",
                                                                          NULL, NULL);
    if (display_config_properties == NULL) {
        cerr << "main(...): g_dbus_proxy_new_for_bus_sync(...) failed." << endl;
        return EXIT_FAILURE;
    }
    if (g_signal_connect(display_config_properties, "g-properties-changed", G_CALLBACK(display_config_properties_changed_handler), touchpad_settings) < 1) {
        cerr << "main(...): g_signal_connect(...) failed." << endl;
        return EXIT_FAILURE;
    }
    
    // sync on start
    // also ensures that "send-events" setting is accessed at least once, which is required for the GSettings singal handling to be correctly initialize
    send_events_handler(touchpad_settings, "send-events", NULL);
    
    return EXIT_SUCCESS;
}

void clean_gnome() {
    g_clear_object(&session_manager_properties);
    g_clear_object(&display_config_properties);
    g_clear_object(&touchpad_settings);
}
