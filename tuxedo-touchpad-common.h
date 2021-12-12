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

#pragma once


#include <iostream>

#include <argp.h>

#include <cstdlib>
#include <csignal>

#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

#include <gio/gio.h>

#include "touchpad-control.h"

using std::cout;
using std::cerr;
using std::endl;


void common_gracefull_exit(int signum);

void common_startup(int &lockfile_arg, bool daemon_mode_arg);


// To be defined in both tuxedo-touchpad-switch[-cli|-daemon].cpp
// User will run either but both don't and must not include one another
void gracefull_exit(int signum = 0);
