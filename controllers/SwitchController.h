#ifndef STEAMCONTROLLERSINGER_SWITCHCONTROLLER_H
#define STEAMCONTROLLERSINGER_SWITCHCONTROLLER_H

#include "Controller.h"

#include <vector>
#include <memory>
#include <unistd.h>

#include "../libs/joycon.hpp"
#include "../constants.h"

class SwitchController : public Controller { //both joycons and pro-controllers
private:
    Joycon joycon;

    static constexpr int MIDI_TO_RUMBLE_VAL[] = {
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        5,
        11,
        17,
        21,
        27,
        33,
        37,
        43,
        49,
        53,
        59,
        65,
        69,
        75,
        81,
        85,
        91,
        97,
        101,
        107,
        113,
        117,
        123,
        129,
        133,
        139,
        145,
        149,
        155,
        161,
        165,
        171,
        177,
        181,
        187,
        193,
        197,
        203,
        209,
        213,
        219,
        225,
        229,
        235,
        241,
        245,
        251,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
    };
public:
    static void openAll(std::vector<std::unique_ptr<Controller>>& controllers);

    explicit SwitchController(struct hid_device_info* dev) : joycon{dev} {};

    int numChannels() override;

    bool claim() override;
    void close() override;
    void reclaim() override;
    int playNote(int side, int note, int duration) override;
    void abortNote();
};


#endif //STEAMCONTROLLERSINGER_SWITCHCONTROLLER_H
