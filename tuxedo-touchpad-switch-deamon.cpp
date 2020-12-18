#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <libudev.h>
#include <string.h>
#include <signal.h>

#include <vector>
#include <string>

#include <gio/gio.h>

const char *bus_str(int bus);

static int get_touchpad_hidraw_devices(std::vector<std::string> *devnodes) {
    struct udev *udev_context = udev_new();
    if (!udev_context) {
        perror("udev_new");
        return -1;
    }

    struct udev_enumerate *hidraw_devices = udev_enumerate_new(udev_context);
    if (!hidraw_devices) {
        perror("udev_enumerate_new");
        udev_unref(udev_context);
        return -1;
    }

    if (udev_enumerate_add_match_subsystem(hidraw_devices, "hidraw") < 0) {
        perror("udev_enumerate_add_match_subsystem");
        udev_enumerate_unref(hidraw_devices);
        udev_unref(udev_context);
        return -1;
    }

    if (udev_enumerate_scan_devices(hidraw_devices) < 0) {
        perror("udev_enumerate_scan_devices");
        udev_enumerate_unref(hidraw_devices);
        udev_unref(udev_context);
        return -1;
    }

    struct udev_list_entry *hidraw_devices_iterator = udev_enumerate_get_list_entry(hidraw_devices);
    if (!hidraw_devices_iterator) {
        udev_enumerate_unref(hidraw_devices);
        udev_unref(udev_context);
        return -1;
    }
    struct udev_list_entry *hidraw_device_entry;
    udev_list_entry_foreach(hidraw_device_entry, hidraw_devices_iterator) {
        if (strstr(udev_list_entry_get_name(hidraw_device_entry), "i2c-UNIW0001")) {
            struct udev_device *hidraw_device = udev_device_new_from_syspath(udev_context, udev_list_entry_get_name(hidraw_device_entry));
            if (!hidraw_device) {
                perror("udev_device_new_from_syspath");
                continue;
            }
            
            std::string devnode = udev_device_get_devnode(hidraw_device);
            devnodes->push_back(devnode);
            
            udev_device_unref(hidraw_device);
        }
    }

    udev_enumerate_unref(hidraw_devices);
    udev_unref(udev_context);

    return devnodes->size();;
}

void send_events_handler(GSettings *settings, const char* key, gpointer user_data) {
    const gchar *send_events_string = g_settings_get_string(settings, key);
    if (!send_events_string) {
        perror("g_settings_get_string");
        return;
    }
    
    std::vector<std::string> devnodes;
    int touchpad_count = get_touchpad_hidraw_devices(&devnodes);
    if (touchpad_count < 0) {
        perror("get_touchpad_hidraw_devices");
        return;
    }
    if (touchpad_count == 0) {
        printf("No compatible touchpads found.\n");
        return;
    }
    
    for (auto it = devnodes.begin(); it != devnodes.end(); ++it) {
        int hidraw = open((*it).c_str(), O_RDWR|O_NONBLOCK);
        if (hidraw < 0) {
            perror("open");
            continue;
        }

        char buffer[2] = {0x07, 0x03};
        if (send_events_string[0] == 'd') {
            buffer[1] = 0x00;
        }
        int result = ioctl(hidraw, HIDIOCSFEATURE(sizeof(buffer)/sizeof(buffer[0])), buffer);
        if (result < 0) {
            perror("ioctl");
            close(hidraw);
            continue;
        }

        close(hidraw);
    }
}

void gracefull_exit(int signum) {
    GSettings *settings_touchpad = g_settings_new("org.gnome.desktop.peripherals.touchpad");
    if (!settings_touchpad) {
        perror("g_settings_new");
        exit(EXIT_FAILURE);
    }
    
    std::vector<std::string> devnodes;
    int touchpad_count = get_touchpad_hidraw_devices(&devnodes);
    if (touchpad_count < 0) {
        perror("get_touchpad_hidraw_devices");
        exit(EXIT_FAILURE);
    }
    if (touchpad_count == 0) {
        printf("No compatible touchpads found.\n");
        exit(EXIT_SUCCESS);
    }
    
    for (auto it = devnodes.begin(); it != devnodes.end(); ++it) {
        int hidraw = open((*it).c_str(), O_RDWR|O_NONBLOCK);
        if (hidraw < 0) {
            perror("open");
            continue;
        }

        char buffer[2] = {0x07, 0x03};
        int result = ioctl(hidraw, HIDIOCSFEATURE(sizeof(buffer)/sizeof(buffer[0])), buffer);
        if (result < 0) {
            perror("ioctl");
            close(hidraw);
            continue;
        }

        close(hidraw);
    }
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, gracefull_exit);
    signal(SIGTERM, gracefull_exit);
    
    GSettings *settings_touchpad = g_settings_new("org.gnome.desktop.peripherals.touchpad");
    if (!settings_touchpad) {
        perror("g_settings_new");
        return EXIT_FAILURE;
    }
    
    // sync on start
    send_events_handler(settings_touchpad, "send-events", NULL);
    
    if (g_signal_connect(settings_touchpad, "changed::send-events", G_CALLBACK(send_events_handler), NULL) < 1) {
        perror("g_signal_connect");
        return EXIT_FAILURE;
    }
    
    GMainLoop *app = g_main_loop_new(NULL, TRUE);
    if (!app) {
        perror("g_main_loop_new");
        return EXIT_FAILURE;
    }
    
    g_main_loop_run(app);
    // g_main_loop_run should not return
    perror("g_main_loop_run");
    return EXIT_FAILURE;
}
