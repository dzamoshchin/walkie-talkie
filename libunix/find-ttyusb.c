// engler, cs140e: your code to find the tty-usb device on your laptop.
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include "libunix.h"
#include <limits.h>

#define _SVID_SOURCE
#include <dirent.h>
static const char *ttyusb_prefixes[] = {
	"ttyUSB",	// linux
    "ttyACM",   // linux
	"cu.SLAB_USB", // mac os
    "cu.usbserial" // mac os
};

static int filter(const struct dirent *d) {
    // scan through the prefixes, returning 1 when you find a match.
    // 0 if there is no match. 
    // for (int i = 0; i < sizeof(ttyusb_prefixes)/ sizeof(ttyusb_prefixes[0]); i++) {
    if (strncmp(ttyusb_prefixes[3], d->d_name, strlen(ttyusb_prefixes[3])) == 0) return 1;
    // }
    return 0;
}

char *find_ttyusb_helper(int setting) {
    char *path = "/dev/";
    struct dirent **dirents;
    int num = scandir(path, &dirents, NULL, alphasort);
    if (num < 0) {
        sys_die(scandir, "could not read files in directory %s\n", path);
    }

    char * devices[num];
    int count = 0;
    for (int i = 0; i < num; i++) {
        if (filter(dirents[i])) {
            devices[count] = dirents[i]->d_name;
            count++;
        }
    }

    char *device = devices[0];

    if (count == 0) panic("No devices were found.\n");
    if (count > 1) {
        if (setting == 0) {
            panic("More than one device was found.\n");
        } else {
            int time = setting == 1 ? INT_MIN : INT_MAX;
            for (int i = 0; i < count; i++) {
                char *device_path = malloc(strlen(path) + strlen(devices[i]) + 1);
                strcpy(device_path, path);
                strcat(device_path, devices[i]);
                struct stat s;
                if (stat(device_path, &s) < 0) {
                    sys_die(stats, "could not read device path %s", device_path);
                }
                int time_check = setting == 1 ? s.st_mtime > time : s.st_mtime < time;
                if (time_check) {
                    time = s.st_mtime;
                    device = devices[i];
                }
            }
        }
    }


    char *result = malloc(strlen(path) + strlen(device) + 1);
    strcpy(result, path);
    strcat(result, device);
    
    return result;
}

// find the TTY-usb device (if any) by using <scandir> to search for
// a device with a prefix given by <ttyusb_prefixes> in /dev
// returns:
//  - device name.
// error: panic's if 0 or more than 1 devices.
char *find_ttyusb(void) {
    return find_ttyusb_helper(0);
}

// return the most recently mounted ttyusb (the one
// mounted last).  use the modification time 
// returned by state.
char *find_ttyusb_last(void) {
    return find_ttyusb_helper(1);
}

// return the oldest mounted ttyusb (the one mounted
// "first") --- use the modification returned by
// stat()
char *find_ttyusb_first(void) {
    return find_ttyusb_helper(2);
}
