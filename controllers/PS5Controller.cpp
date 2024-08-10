#include "PS5Controller.h"

#include <algorithm>

#include <unistd.h>
#include <fcntl.h>

#include <hidapi.h>

void PS5Controller::openAll(std::vector<std::unique_ptr<Controller>>& controllers) {
    // Enumerate HID devices on the system
    struct hid_device_info *devs, *cur_dev;
    int res = hid_init();

    devs = hid_enumerate(DS5_VENDOR_ID, 0x0);
    cur_dev = devs;

    // Linear search for joycons
    std::vector<std::string> pathsSeen{}; //for some reason the controllers don't disconnect properly and create copies with the same path
    while (cur_dev)
    {
        // identify by vendor:
        if (cur_dev->vendor_id == DS5_VENDOR_ID) {
            if (std::find(pathsSeen.begin(), pathsSeen.end(), cur_dev->path) != pathsSeen.end()) {
                cur_dev = cur_dev->next;
                continue;
            }
            pathsSeen.emplace_back(cur_dev->path);

            // Dualsense 5:
            if (cur_dev->product_id == DS5_PRODUCT_ID) {
                controllers.emplace_back(std::make_unique<PS5Controller>(cur_dev->path))->claim();
            }
        }

        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);
}

PS5Controller::PS5Controller(const char* path) {
    fd = open(path, O_RDWR|O_NONBLOCK);

    unsigned char bufWrite[66] = {};

    unsigned char rumblePowerLeft = 0xFF;
    unsigned char rumblePowerRight = 0x00;
    unsigned char ledState = 0xFF;
    unsigned char colourR = 0x80;
    unsigned char colourG = 0x00;
    unsigned char colourB = 0xFF;

    /* Send a Report to the Device */
    bufWrite[0] = 0x15;
    bufWrite[1] = 0xff;
    bufWrite[2] = 0xf4;  // 0xff disable all LEDs - top LED, bottom LED, Mic LED (this just if bufWrite[63] == 0x02)// 0xf3 disable top LED// 0xf4 enable all LEDs - top LED, bottom LED, Mic LED (this just if buf[63] == 0x02)
    bufWrite[3] = rumblePowerRight;  // Right motor power
    bufWrite[4] = rumblePowerLeft;  // Left motor power
    /* ... */
    bufWrite[9] = 0x00;  // Blinking Mic LED (if buf[2] == f7)
    /* ... */
    bufWrite[44] = ledState; // LEDs from left to right -> 0xc1 0xa2 0xc4 0xc8 0xd0// 0x00 all OFF, 0xff all ON
    bufWrite[45] = colourR; // R
    bufWrite[46] = colourG; // G
    bufWrite[47] = colourB; // B
    /* ... */
    bufWrite[63] = 0x02; // Short Blink bottom led
    /* ... */
    bufWrite[65] = 0x02; // If both are set to 0xff it will turn OFF Motors and LEDs
//    bufWrite[66] = 0x02; // else if both are set 0x02 it will turn ON Motors and LEDs

    ssize_t res = write(fd, bufWrite, sizeof(bufWrite));
    if (res < 0) {
        throw std::runtime_error{"DS5 write failed"};
    }
}

bool PS5Controller::claim() {
    return false;
}

void PS5Controller::close() {

}

void PS5Controller::reclaim() {

}

int PS5Controller::playNote(int side, int note, int duration) {
    return 0;
}

void PS5Controller::abortNote() {

}
