#ifndef CONTROLLERORCHESTRA_PS5CONTROLLER_H
#define CONTROLLERORCHESTRA_PS5CONTROLLER_H

#include "Controller.h"

#include <vector>
#include <memory>

class PS5Controller : public Controller { //also called a dualsense 5
private:
    static constexpr int DS5_VENDOR_ID = 1356;
    static constexpr int DS5_PRODUCT_ID = 3302;

    int fd;
public:
    static void openAll(std::vector<std::unique_ptr<Controller>>& controllers);

    PS5Controller(const char* path);

    int numChannels() override { return 2; };

    bool claim() override;
    void close() override;
    void reclaim() override;
    int playNote(int side, int note, int duration) override;
    void abortNote() override;
};


#endif //CONTROLLERORCHESTRA_PS5CONTROLLER_H
