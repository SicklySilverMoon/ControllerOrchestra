#ifndef CONTROLLERORCHESTRA_CONTROLLER_H
#define CONTROLLERORCHESTRA_CONTROLLER_H

#include "../constants.h"

class Controller {
public:
    virtual int numChannels() = 0;

    virtual bool claim() = 0;
    virtual void close() = 0;
    virtual void reclaim() = 0;
    virtual int playNote(int side, int note, int duration) = 0;
    virtual void abortNote() = 0;

    virtual ~Controller() = default;
};


#endif //CONTROLLERORCHESTRA_CONTROLLER_H
