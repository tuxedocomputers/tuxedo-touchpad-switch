#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <libudev.h>
#include <string.h>

#include <vector>
#include <string>

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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <enable|disable>\n", argv[0]);
        return EXIT_SUCCESS;
    }
    
    std::vector<std::string> devnodes;
    int touchpad_count = get_touchpad_hidraw_devices(&devnodes);
    if (touchpad_count < 0) {
        return EXIT_FAILURE;
    }
    if (touchpad_count == 0) {
        printf("No compatible touchpads found.\n");
        return EXIT_SUCCESS;
    }
    
    int result = EXIT_SUCCESS;
    for (auto it = devnodes.begin(); it != devnodes.end(); ++it) {
        int hidraw = open((*it).c_str(), O_RDWR|O_NONBLOCK);
        if (hidraw < 0) {
            perror("open");
            result = EXIT_FAILURE;
            continue;
        }

        char buffer[2] = {0x07, 0x03};
        if (argv[1][0] == 'd') {
            buffer[1] = 0x00;
        }
        int result = ioctl(hidraw, HIDIOCSFEATURE(sizeof(buffer)/sizeof(buffer[0])), buffer);
        if (result < 0) {
            perror("ioctl");
            close(hidraw);
            result = EXIT_FAILURE;
            continue;
        }
        else {
            printf("ioctl returned: %d\n", result);
        }

        close(hidraw);
    }
    
    return result;
}
