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


static int lockfile = -1;

void gracefull_exit(int signum) {
    common_gracefull_exit(signum);
}


static int parse_opt(int key, char *arg, struct argp_state *state) {
    switch (key) {
    case 'g': {
        int current_state = get_touchpad_state();
        if(current_state < 0){
            gracefull_exit(current_state);
        }
        switch (current_state)
        {
        case TOUCHPAD_ENABLE:
            cout << "on" << endl;
            break;
        case TOUCHPAD_DISABLE:
            cout << "off" << endl;
            break;
        case TOUCHPAD_TOUCH_DISABLE:
            cout << "touch" << endl;
            break;
        default:
            cerr << "unknown keyboard state:" << current_state << endl;
            gracefull_exit(EXIT_FAILURE);
            break;
        }

        gracefull_exit(EXIT_SUCCESS);
    }
    break;
    case 's': {
            long state = -1;
            
            if(strcmp(arg, "on") == 0){
                state = TOUCHPAD_ENABLE;
            }
            if(strcmp(arg, "touch") == 0){
                state = TOUCHPAD_TOUCH_DISABLE;
            }
            if(strcmp(arg, "off") == 0){
                state = TOUCHPAD_DISABLE;
            }

            state = state == -1? strtol(arg, NULL, 0) : state;
            if(state > UINT8_MAX || state < 0){
                cerr << "Value provided out of bounds for state. Must fit an unsigned byte" << endl;
                gracefull_exit(-EXIT_FAILURE);
            }
            int result = set_touchpad_state(state);
            if (result < 0){
                gracefull_exit(-EXIT_FAILURE);
            }
            gracefull_exit(EXIT_SUCCESS);
        }
        break;
    case 't': {
           int new_state = toggle_touchpad_state();
           if(new_state < 0){
               cerr << "Failed to set state as '" << arg << "'" << endl;
               gracefull_exit(new_state);
           }
           cout <<  (new_state ? "on" : "off") << endl;
           gracefull_exit(EXIT_SUCCESS);
        }
        break;
    }
    return 0;
}

int main(int argc, char** argv) {

    common_startup(lockfile, false);

    struct argp_option options[] = {
        { "get", 'g', NULL, 0, "Get the touchpad state. Values are off, touch, on. off should be returned when LED is on"},
        { "set", 's', "state", 0, "Set the touchpad state. The three possible values are off, touch, on. LED will be on when off is used"},
        { "toggle", 't', NULL, OPTION_ARG_OPTIONAL, "Toggle touchpad between on and off"},
        { 0 }
    };
    struct argp argp = { options, parse_opt };
    int argp_response = argp_parse (&argp, argc, argv, 0, 0, 0);
    if(argp_response != 0){
        gracefull_exit(argp_response);
    }

    gracefull_exit(-EXIT_FAILURE);
}
