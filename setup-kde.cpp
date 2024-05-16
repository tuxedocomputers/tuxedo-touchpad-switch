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

#include "setup-kde.h"

#include <iostream>

#include <sys/file.h>

#include <gio/gio.h>

#include "touchpad-control.h"

using std::cerr;
using std::endl;

int lockfile;
gboolean isMousePluggedInPrev;
gboolean isEnabledSave;
GDBusProxy *kded_modules_touchpad = NULL;
GDBusProxy *solid_power_management = NULL;

static void kded_modules_touchpad_handler(GDBusProxy *proxy, __attribute__((unused)) char *sender_name, char *signal_name, GVariant *parameters, __attribute__((unused)) gpointer user_data) {
    if (!strcmp("enabledChanged", signal_name) && g_variant_is_of_type(parameters, (const GVariantType *)"(b)") && g_variant_n_children(parameters)) {
        GVariant *enabledChanged = g_variant_get_child_value(parameters, 0);
        
        isEnabledSave = g_variant_get_boolean(enabledChanged);
        if (isEnabledSave) {
            if (set_touchpad_state(1)) {
                cerr << "kded_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
            }
        }
        else {
            if (set_touchpad_state(0)) {
                cerr << "kded_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
            }
        }
        g_variant_unref(enabledChanged);
    }
    else if (!strcmp("mousePluggedInChanged", signal_name) && g_variant_is_of_type(parameters, (const GVariantType *)"(b)") && g_variant_n_children(parameters)) {
        GVariant *isMousePluggedInParam = g_dbus_proxy_call_sync(proxy, "isMousePluggedIn", NULL, G_DBUS_CALL_FLAGS_NONE, G_MAXINT, NULL, NULL);
        if (isMousePluggedInParam != NULL && g_variant_is_of_type(isMousePluggedInParam, (const GVariantType *)"(b)") && g_variant_n_children(isMousePluggedInParam)) {
            GVariant *isMousePluggedIn = g_variant_get_child_value(isMousePluggedInParam, 0);
            if (isMousePluggedInPrev && !g_variant_get_boolean(isMousePluggedIn)) {
                if (set_touchpad_state(1)) {
                    cerr << "kded_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
                }
                if (flock(lockfile, LOCK_UN)) {
                    cerr << "kded_modules_touchpad_handler(...): flock(...) failed." << endl;
                }
            }
            else if (!isMousePluggedInPrev && g_variant_get_boolean(isMousePluggedIn)) {
                if (flock(lockfile, LOCK_EX)) {
                    cerr << "kded_modules_touchpad_handler(...): flock(...) failed." << endl;
                }
                if (set_touchpad_state(isEnabledSave)) {
                    cerr << "kded_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
                }
            }
            isMousePluggedInPrev = g_variant_get_boolean(isMousePluggedIn);
            g_variant_unref(isMousePluggedIn);
            g_variant_unref(isMousePluggedInParam);
        }
        else {
            cerr << "kded_modules_touchpad_handler(...): g_dbus_proxy_call_sync(...) failed." << endl;
        }
    }
}

static void solid_power_management_handler(__attribute__((unused)) GDBusProxy *proxy, __attribute__((unused)) char *sender_name, char *signal_name, __attribute__((unused)) GVariant *parameters, __attribute__((unused)) gpointer user_data) {
    if (!strcmp("aboutToSuspend", signal_name)) {
        if (set_touchpad_state(1)) {
            cerr << "kded_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
        }
        if (flock(lockfile, LOCK_UN)) {
            cerr << "kded_modules_touchpad_handler(...): flock(...) failed." << endl;
        }
    }
    else if (!strcmp("resumingFromSuspend", signal_name)) {
        if (flock(lockfile, LOCK_EX)) {
            cerr << "kded_modules_touchpad_handler(...): flock(...) failed." << endl;
        }
        if (set_touchpad_state(isEnabledSave)) {
            cerr << "kded_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
        }
    }
}

static int kded_modules_touchpad_init(GDBusProxy *proxy) {
    GVariant *isEnabledParam = g_dbus_proxy_call_sync(proxy, "isEnabled", NULL, G_DBUS_CALL_FLAGS_NONE, G_MAXINT, NULL, NULL);
    if (isEnabledParam != NULL && g_variant_is_of_type(isEnabledParam, (const GVariantType *)"(b)") && g_variant_n_children(isEnabledParam)) {
        GVariant *isEnabled = g_variant_get_child_value(isEnabledParam, 0);

        // init isEnabledSave
        isEnabledSave = g_variant_get_boolean(isEnabled);
        if (isEnabledSave) {
            if (set_touchpad_state(1)) {
                cerr << "kded_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
                return EXIT_FAILURE;
            }
        }
        else {
            if (set_touchpad_state(0)) {
                cerr << "kded_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
                return EXIT_FAILURE;
            }
        }

        g_variant_unref(isEnabled);
        g_variant_unref(isEnabledParam);
    }
    else {
        cerr << "kded_modules_touchpad_handler(...): g_dbus_proxy_call_sync(...) failed." << endl;
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
                cerr << "kded_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
                return EXIT_FAILURE;
            }
            if (flock(lockfile, LOCK_UN)) {
                cerr << "kded_modules_touchpad_handler(...): flock(...) failed." << endl;
                return EXIT_FAILURE;
            }
        }
        
        g_variant_unref(isMousePluggedIn);
        g_variant_unref(isMousePluggedInParam);
    }
    else {
        cerr << "kded_modules_touchpad_handler(...): g_dbus_proxy_call_sync(...) failed." << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int setup_kde(int lockfile_arg) {
    lockfile = lockfile_arg;

    kded_modules_touchpad = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                          G_DBUS_PROXY_FLAGS_NONE, NULL,
                                                          "org.kde.kded6",
                                                          "/modules/kded_touchpad",
                                                          "org.kde.touchpad",
                                                          NULL, NULL);
    // call a random method to check if object exists
    GVariant *isMousePluggedInParam = g_dbus_proxy_call_sync(kded_modules_touchpad,
                                                             "isMousePluggedIn",
                                                             NULL, G_DBUS_CALL_FLAGS_NONE,
                                                             G_MAXINT, NULL, NULL);
    if (isMousePluggedInParam == NULL) {
        g_object_unref(kded_modules_touchpad);
        g_variant_unref(isMousePluggedInParam);
        kded_modules_touchpad = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                              G_DBUS_PROXY_FLAGS_NONE, NULL,
                                                              "org.kde.kded5",
                                                              "/modules/kded_touchpad",
                                                              "org.kde.touchpad",
                                                              NULL, NULL);
        // call a random method to check if object exists
        isMousePluggedInParam = g_dbus_proxy_call_sync(kded_modules_touchpad,
                                                                 "isMousePluggedIn",
                                                                 NULL, G_DBUS_CALL_FLAGS_NONE,
                                                                 G_MAXINT, NULL, NULL);
        if (isMousePluggedInParam == NULL) {
            g_object_unref(kded_modules_touchpad);
            g_variant_unref(isMousePluggedInParam);
            kded_modules_touchpad = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                                  G_DBUS_PROXY_FLAGS_NONE, NULL,
                                                                  "org.kde.kded5",
                                                                  "/modules/touchpad",
                                                                  "org.kde.touchpad",
                                                                  NULL, NULL);
            // call a random method to check if object exists
            isMousePluggedInParam = g_dbus_proxy_call_sync(kded_modules_touchpad,
                                                                    "isMousePluggedIn",
                                                                    NULL, G_DBUS_CALL_FLAGS_NONE,
                                                                    G_MAXINT, NULL, NULL);
            if (isMousePluggedInParam == NULL) {
                g_clear_object(&kded_modules_touchpad);
                g_variant_unref(isMousePluggedInParam);
                cerr << "setup_kde(...): g_dbus_proxy_new_for_bus_sync(...) failed." << endl;
                return EXIT_FAILURE;
            }
        }
    }
    g_variant_unref(isMousePluggedInParam);

    if (g_signal_connect(kded_modules_touchpad, "g-signal", G_CALLBACK(kded_modules_touchpad_handler), NULL) < 1) {
        cerr << "setup_kde(...): g_signal_connect(...) failed." << endl;
        g_clear_object(&kded_modules_touchpad);
        return EXIT_FAILURE;
    }
    
    // sync on wakeup
    solid_power_management = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                                       G_DBUS_PROXY_FLAGS_NONE, NULL,
                                                                       "org.kde.Solid.PowerManagement",
                                                                       "/org/kde/Solid/PowerManagement/Actions/SuspendSession",
                                                                       "org.kde.Solid.PowerManagement.Actions.SuspendSession",
                                                                       NULL, NULL);
    if (solid_power_management == NULL) {
        cerr << "setup_kde(...): g_dbus_proxy_new_for_bus_sync(...) failed." << endl;
        g_clear_object(&kded_modules_touchpad);
        return EXIT_FAILURE;
    }
    if (g_signal_connect(solid_power_management, "g-signal", G_CALLBACK(solid_power_management_handler), NULL) < 1) {
        cerr << "setup_kde(...): g_signal_connect(...) failed." << endl;
        clean_kde();
        return EXIT_FAILURE;
    }
    
    // sync on start
    if (kded_modules_touchpad_init(kded_modules_touchpad) == EXIT_FAILURE) {
        cerr << "setup_kde(...): kded_modules_touchpad_init(...) failed." << endl;
        clean_kde();
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

void clean_kde() {
    g_clear_object(&kded_modules_touchpad);
    g_clear_object(&solid_power_management);
}
