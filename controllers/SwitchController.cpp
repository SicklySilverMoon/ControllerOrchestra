#include "SwitchController.h"

#include <algorithm>

#include <hidapi.h>

void SwitchController::openAll(std::vector<std::unique_ptr<Controller>>& controllers) {
    // Enumerate HID devices on the system
    struct hid_device_info *devs, *cur_dev;
    int res = hid_init();

    devs = hid_enumerate(JOYCON_VENDOR, 0x0);
    cur_dev = devs;

    // Linear search for joycons
    std::vector<std::string> pathsSeen{}; //for some reason the controllers don't disconnect properly and create copies with the same path
    while (cur_dev)
    {
        // identify by vendor:
        if (cur_dev->vendor_id == JOYCON_VENDOR) {
            if (std::find(pathsSeen.begin(), pathsSeen.end(), cur_dev->path) != pathsSeen.end()) {
                cur_dev = cur_dev->next;
                continue;
            }
            pathsSeen.emplace_back(cur_dev->path);

            // bluetooth, left / right joycon & pro controller:
            if (cur_dev->product_id == PRO_CONTROLLER || cur_dev->product_id == JOYCON_L_BT || cur_dev->product_id == JOYCON_R_BT) {
                controllers.emplace_back(std::make_unique<SwitchController>(cur_dev))->claim();
            }
        }

        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);
}

int SwitchController::numChannels() {
    if (joycon.control_type == Joycon::TYPE::TYPE_PRO_CONTROLLER) {
        return 2;
    } else {
        return 1;
    }
}

bool SwitchController::claim() {
    joycon.init_bt();
    return true;
}

void SwitchController::close() {

}

void SwitchController::reclaim() {

}

int SwitchController::playNote(int side, int note, int duration) {
    if (note == NOTE_STOP) {
        joycon.rumble(100, 3);
        return 0;
    }

    int rumble = MIDI_TO_RUMBLE_VAL[note];

    if (joycon.control_type != Joycon::TYPE::TYPE_PRO_CONTROLLER) {
        joycon.rumble(rumble, 1);
    } else {
        if (side == 0) {
            joycon.rumble_l(rumble, 1);
        } else {
            joycon.rumble_r(rumble, 1);
        }
    }

    return 0;
}

void SwitchController::abortNote() {
    joycon.rumble(100, 3);
}
