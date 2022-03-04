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
GDBusProxy *kded5_modules_touchpad = NULL;
GDBusProxy *solid_power_management = NULL;

static void kded5_modules_touchpad_handler(GDBusProxy *proxy, __attribute__((unused)) char *sender_name, char *signal_name, GVariant *parameters, __attribute__((unused)) gpointer user_data) {
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
            if (isMousePluggedInPrev && !g_variant_get_boolean(isMousePluggedIn)) {
                if (set_touchpad_state(1)) {
                    cerr << "kded5_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
                }
                if (flock(lockfile, LOCK_UN)) {
                    cerr << "kded5_modules_touchpad_handler(...): flock(...) failed." << endl;
                }
            }
            else if (!isMousePluggedInPrev && g_variant_get_boolean(isMousePluggedIn)) {
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

static void solid_power_management_handler(__attribute__((unused)) GDBusProxy *proxy, __attribute__((unused)) char *sender_name, char *signal_name, __attribute__((unused)) GVariant *parameters, __attribute__((unused)) gpointer user_data) {
    if (!strcmp("aboutToSuspend", signal_name)) {
        if (set_touchpad_state(1)) {
            cerr << "kded5_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
        }
        if (flock(lockfile, LOCK_UN)) {
            cerr << "kded5_modules_touchpad_handler(...): flock(...) failed." << endl;
        }
    }
    else if (!strcmp("resumingFromSuspend", signal_name)) {
        if (flock(lockfile, LOCK_EX)) {
            cerr << "kded5_modules_touchpad_handler(...): flock(...) failed." << endl;
        }
        if (set_touchpad_state(isEnabledSave)) {
            cerr << "kded5_modules_touchpad_handler(...): set_touchpad_state(...) failed." << endl;
        }
    }
}

static int kded5_modules_touchpad_init(GDBusProxy *proxy) {
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

int setup_kde(int lockfile_arg) {
    lockfile = lockfile_arg;
    const char *object_path = NULL;

    GDBusProxy *kded5 = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                      G_DBUS_PROXY_FLAGS_NONE, NULL,
                                                      "org.kde.kded5",
                                                      "/kded",
                                                      "org.kde.kded5",
                                                      NULL, NULL);
    if (kded5 == NULL) {
        cerr << "setup_kde(...): g_dbus_proxy_new_for_bus_sync(...) failed." << endl;
        return EXIT_FAILURE;
    }
    GVariant *loadedModulesParam = g_dbus_proxy_call_sync(kded5, "loadedModules", NULL, G_DBUS_CALL_FLAGS_NONE, G_MAXINT, NULL, NULL);
    if (loadedModulesParam != NULL && g_variant_is_of_type(loadedModulesParam, (const GVariantType *)"(as)") && g_variant_n_children(loadedModulesParam) ) {
        GVariant *loadedModules = g_variant_get_child_value(loadedModulesParam, 0);
        GVariantIter *iter;
        gchar *str;

        g_variant_get(loadedModules, "as", &iter);
        while (g_variant_iter_loop(iter, "s", &str)) {
            if (strcmp(str, "kded_touchpad") == 0) {
                object_path = "/modules/kded_touchpad";
                break; // break is fine here, we dont need to free unpacked strings
            }
            if (strcmp(str, "touchpad") == 0) {
                object_path = "/modules/touchpad";
                break; // break is fine here, we dont need to free unpacked strings
            }
        }

        g_variant_iter_free(iter);
        g_variant_unref(loadedModules);
        g_variant_unref(loadedModulesParam);
    } else {
        g_object_unref(kded5); // not needed anymore
        cerr << "setup_kde(...): g_dbus_proxy_call_sync(...) failed." << endl;
        return EXIT_FAILURE;
    }
    g_object_unref(kded5); // not needed anymore
    if (object_path == NULL) {
        cerr << "setup_kde(...): Could not find touchpad module in kded." << endl;
        return EXIT_FAILURE;
    }

    // sync on config and xsession change
    kded5_modules_touchpad = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                                       G_DBUS_PROXY_FLAGS_NONE, NULL,
                                                                       "org.kde.kded5",
                                                                       object_path,
                                                                       "org.kde.touchpad",
                                                                       NULL, NULL);
    if (kded5_modules_touchpad == NULL) {
        cerr << "setup_kde(...): g_dbus_proxy_new_for_bus_sync(...) failed." << endl;
        return EXIT_FAILURE;
    }
    if (g_signal_connect(kded5_modules_touchpad, "g-signal", G_CALLBACK(kded5_modules_touchpad_handler), NULL) < 1) {
        cerr << "setup_kde(...): g_signal_connect(...) failed." << endl;
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
        return EXIT_FAILURE;
    }
    if (g_signal_connect(solid_power_management, "g-signal", G_CALLBACK(solid_power_management_handler), NULL) < 1) {
        cerr << "setup_kde(...): g_signal_connect(...) failed." << endl;
        return EXIT_FAILURE;
    }
    
    // sync on start
    if (kded5_modules_touchpad_init(kded5_modules_touchpad) == EXIT_FAILURE) {
        cerr << "setup_kde(...): kded5_modules_touchpad_init(...) failed." << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

void clean_kde() {
    g_clear_object(&kded5_modules_touchpad);
    g_clear_object(&solid_power_management);
}
