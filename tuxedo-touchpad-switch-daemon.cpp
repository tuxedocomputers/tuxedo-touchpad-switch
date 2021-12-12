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


#include "tuxedo-touchpad-common.h"

#include "setup-gnome.h"
#include "setup-kde.h"

static int lockfile = -1;

void gracefull_exit(int signum) {
    clean_gnome();
    clean_kde();

    common_gracefull_exit(signum);
}


int main(int argc, char** argv) {

    common_startup(lockfile, true);
    
    char *xdg_current_desktop = getenv("XDG_CURRENT_DESKTOP");
    if (strstr(xdg_current_desktop, "GNOME")) {
        setup_gnome(lockfile);
    }
    else if (strstr(xdg_current_desktop, "KDE")) {
        setup_kde(lockfile);
    }
    else {
        cout << "Your desktop environment is not supported in daemon mode." << endl;
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
