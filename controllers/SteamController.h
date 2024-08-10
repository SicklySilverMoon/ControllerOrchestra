#ifndef CONTROLLERORCHESTRA_STEAMCONTROLLER_H
#define CONTROLLERORCHESTRA_STEAMCONTROLLER_H

#include "Controller.h"

#include <libusb.h>

#include "../constants.h"

class SteamController : public Controller {
private:
    libusb_device_handle* dev_handle;
    int interfaceNum;

    static constexpr double STEAM_CONTROLLER_MAGIC_PERIOD_RATIO = 495483.0;

    static inline int instances = 0;
public:
    SteamController();

    int numChannels() override { return 2; };

    bool claim() override;
    void close() override;
    void reclaim() override;
    int playNote(int side, int note, int duration) override;
    void abortNote() override;
};


#endif //CONTROLLERORCHESTRA_STEAMCONTROLLER_H
