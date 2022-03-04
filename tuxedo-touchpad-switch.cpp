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

#include <iostream>

#include <cstdlib>
#include <csignal>

#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

#include <gio/gio.h>

#include "touchpad-control.h"
#include "setup-gnome.h"
#include "setup-kde.h"

using std::cout;
using std::cerr;
using std::endl;

static int lockfile = -1;

static void gracefull_exit(int signum = 0) {
    int result = EXIT_SUCCESS;
    
    clean_gnome();
    clean_kde();
    
    if (signum < 0) {
        result = EXIT_FAILURE;
    }
    
    if (set_touchpad_state(1) != EXIT_SUCCESS) {
        cerr << "gracefull_exit(...): set_touchpad_state(...) failed." << endl;
        result = EXIT_FAILURE;
    }
    
    if (lockfile >= 0) {
        if (flock(lockfile, LOCK_UN)) {
            cerr << "gracefull_exit(...): flock(...) failed." << endl;
            result = EXIT_FAILURE;
        }
        if (close(lockfile)) {
            cerr << "gracefull_exit(...): close(...) failed." << endl;
            result = EXIT_FAILURE;
        }
    }
    
    exit(result);
}

int main() {
    struct sigaction sigaction_gracefull_exit;
    sigaction_gracefull_exit.sa_handler = gracefull_exit;
    sigemptyset(&sigaction_gracefull_exit.sa_mask);
    sigaction_gracefull_exit.sa_flags = 0;
    
    if (sigaction(SIGINT, &sigaction_gracefull_exit, nullptr)) {
        cerr << "main(...): sigaction(...) failed." << endl;
        gracefull_exit(-EXIT_FAILURE);
    }
    if (sigaction(SIGTERM, &sigaction_gracefull_exit, nullptr)) {
        cerr << "main(...): sigaction(...) failed." << endl;
        gracefull_exit(-EXIT_FAILURE);
    }
    if (sigaction(SIGHUP, &sigaction_gracefull_exit, nullptr)) {
        cerr << "main(...): sigaction(...) failed." << endl;
        gracefull_exit(-EXIT_FAILURE);
    }
    
    lockfile = open("/etc/tuxedo-touchpad-switch-lockfile", O_RDONLY);
    if (lockfile == -1) {
        cerr << "main(...): open(...) failed." << endl;
        gracefull_exit(-EXIT_FAILURE);
    }
    
    if (flock(lockfile, LOCK_EX) == -1) {
        cerr << "main(...): flock(...) failed." << endl;
        gracefull_exit(-EXIT_FAILURE);
    }
    
    char *xdg_current_desktop = getenv("XDG_CURRENT_DESKTOP");
    if (!xdg_current_desktop) {
        cout << "Your desktop environment could not be determined." << endl;
        gracefull_exit(SIGTERM);
    }
    else if (strstr(xdg_current_desktop, "GNOME")) {
        int ret = setup_gnome(lockfile);
        if (ret != EXIT_SUCCESS) {
            cerr << "main(...): setup_gnome(...) failed." << endl;
            gracefull_exit(-ret);
        }
    }
    else if (strstr(xdg_current_desktop, "KDE")) {
        int ret = setup_kde(lockfile);
        if (ret != EXIT_SUCCESS) {
            cerr << "main(...): setup_kde(...) failed." << endl;
            gracefull_exit(-ret);
        }
    }
    else {
        cout << "Your desktop environment is not supported." << endl;
        gracefull_exit(SIGTERM);
    }
    
    // start empty glib mainloop, required for glib signals to be catched
    GMainLoop *app = g_main_loop_new(NULL, TRUE);
    if (!app) {
        cerr << "main(...): g_main_loop_new(...) failed." << endl;
        gracefull_exit(-EXIT_FAILURE);
    }
    
    g_main_loop_run(app);
    // g_main_loop_run only returns on error
    g_clear_object(&app);
    cerr << "main(...): g_main_loop_run(...) failed." << endl;
    gracefull_exit(-EXIT_FAILURE);
}
