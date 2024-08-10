#include "SteamController.h"

#include <iostream>
#include <stdexcept>

SteamController::SteamController() {
    if (instances == 0) {
        int r = libusb_init(nullptr);
        if (r < 0) {
            throw std::runtime_error{"LIBUSB Init Error"};
        }
        libusb_set_debug(nullptr, LIBUSB_LOG_LEVEL_NONE);
    }

    //Open Steam Controller device
    //todo: swap this to libusb_get_device_list and a proper enumeration of it to open all steam controllers (not relevant for us rn)
    //  actually, see if this can't be swapped to hidapi instead to unify the libs used for switch controllers and steam controllers
    if((dev_handle = libusb_open_device_with_vid_pid(nullptr, 0x28DE, 0x1102)) != nullptr) { // Wired Steam Controller
        std::cout << "Found wired Steam Controller" << std::endl;
        interfaceNum = 2;
    } else if((dev_handle = libusb_open_device_with_vid_pid(nullptr, 0x28DE, 0x1142)) != nullptr) { // Steam Controller dongle
        std::cout << "Found Steam Dongle, will attempt to use the first Steam Controller" << std::endl;
        interfaceNum = 1;
    } else if ((dev_handle = libusb_open_device_with_vid_pid(nullptr, 0x28DE, 0x1205)) != nullptr) { // Steam Deck
        std::cout << "Found Steam Deck built-in controller" << std::endl;
        interfaceNum = 2;
    } else {
        throw std::runtime_error{"No device found"};
    }

    //On Linux, automatically detach and reattach kernel module
    libusb_set_auto_detach_kernel_driver(dev_handle,1);

    if (!claim()) {
        throw std::runtime_error{"Failed to claim controller"};
    }
}

bool SteamController::claim() {
    //Claim the USB interface controlling the haptic actuators
    int r = libusb_claim_interface(dev_handle,interfaceNum);
    if(r < 0) {
        std::cout << "Interface claim Error " << r << std::endl;
        std::cin.ignore();
        libusb_close(dev_handle);
        return false;
    }

    return true;
}

void SteamController::close() {
    int r = libusb_release_interface(dev_handle,interfaceNum);
    if (r < 0) {
        std::cout << "Interface release Error " << r << std::endl;
        std::cin.ignore();
        return;
    }
    libusb_close(dev_handle);

    instances--;
    if (instances == 0) {
        libusb_exit(nullptr);
    }
}

void SteamController::reclaim() {
    int r = libusb_release_interface(dev_handle,interfaceNum);
    if (r < 0) {
        std::cout << "Interface release Error " << r << std::endl;
        std::cin.ignore();
        return;
    }
    claim();
}

int SteamController::playNote(int side, int note, int duration) {
    unsigned char dataBlob[64] = {0x8f,
                                  0x07,
                                  0x00, //Trackpad select : 0x01 = left, 0x00 = right
                                  0xff, //LSB Pulse High Duration
                                  0xff, //MSB Pulse High Duration
                                  0xff, //LSB Pulse Low Duration
                                  0xff, //MSB Pulse Low Duration
                                  0xff, //LSB Pulse repeat count
                                  0x04, //MSB Pulse repeat count
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    if(note == NOTE_STOP){
        note = 0;
        duration = 0.0;
    }

    double frequency = midiFrequency[note];
    double period = 1.0 / frequency;
    uint16_t periodCommand = period * STEAM_CONTROLLER_MAGIC_PERIOD_RATIO;

    //Compute number of repeat. If duration < 0, set to maximum
    uint16_t repeatCount = (duration >= 0.0) ? (duration / period) : 0x7FFF;

    //cout << "Frequency : " <<frequency << ", Period : "<<periodCommand << ", Repeat : "<< repeatCount <<"\n";

    dataBlob[2] = side;
    dataBlob[3] = periodCommand % 0xff;
    dataBlob[4] = periodCommand / 0xff;
    dataBlob[5] = periodCommand % 0xff;
    dataBlob[6] = periodCommand / 0xff;
    dataBlob[7] = repeatCount % 0xff;
    dataBlob[8] = repeatCount / 0xff;

    int r;
    r = libusb_control_transfer(dev_handle,0x21,9,0x0300,2,dataBlob,64,1000);
    if(r < 0) {
        std::cout << "Command Error " << r << std::endl;
        exit(0);
    }

    return 0;
}

void SteamController::abortNote() {
    for(int i = 0 ; i < 2 ; i++){
        playNote(i, NOTE_STOP, DURATION_MAX);
    }
}
