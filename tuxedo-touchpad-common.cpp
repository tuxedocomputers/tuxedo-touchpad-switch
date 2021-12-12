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

#include <argp.h>

#include <cstdlib>
#include <csignal>

#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

#include <gio/gio.h>

#include "tuxedo-touchpad-common.h"
#include "touchpad-control.h"
#include "setup-gnome.h"
#include "setup-kde.h"

using std::cout;
using std::cerr;
using std::endl;


static int lockfile = -1;
static bool daemon_mode = true;

void common_gracefull_exit(int signum = 0) {
    int result = EXIT_SUCCESS;
    
    
    if (signum < 0) {
        result = EXIT_FAILURE;
    }
    
    if (daemon_mode && set_touchpad_state(TOUCHPAD_ENABLE) != EXIT_SUCCESS) {
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


void common_startup(int &lockfile_arg, bool daemon_mode_arg){

    daemon_mode = daemon_mode_arg;

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
    
    lockfile = open(PID_FILE_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (lockfile == -1) {
        perror("main(...): open(...) failed");
        gracefull_exit(-EXIT_FAILURE);
    }
    lockfile_arg = lockfile;   
    
    if (flock(lockfile, LOCK_EX | LOCK_NB) == -1) {
        switch(errno){
            case EWOULDBLOCK:
                char pid_s[12] = {0};
                cerr << "tuxedo-touchpad-switch is already running (possibly as a daemon) with ";
                if(read(lockfile, pid_s, (sizeof(pid_s) - 2) * sizeof(char) > 0 )) {
                    cerr << "pid " << pid_s << endl;
                } else {
                    cerr << "unknown pid " << endl;
                }
                
                gracefull_exit(-EXIT_FAILURE);
        }
        perror("main(...): flock(...) failed");
        gracefull_exit(-EXIT_FAILURE);
    }

    if (ftruncate(lockfile, 0) < 0){
        perror("main(...): lseek(...) failed");
        gracefull_exit(-EXIT_FAILURE);
    }

    char pidbuffer[10];
    int written_size = snprintf(pidbuffer, sizeof(pidbuffer), "%d", getpid());

    if (write (lockfile, pidbuffer, written_size) < 0){
        perror("main(...): write(...) failed. Continuing anyway");
    }

}
