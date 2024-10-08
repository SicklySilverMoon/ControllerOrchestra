#include <iostream>
#include <chrono>
#include <functional>
#include <memory>

#include <stdint-gcc.h>
#include <unistd.h>
#include <cstdint>

#include <csignal>
#include <cstdio>

#include <libusb.h>

#include "midifile/midifile.h"

#include "constants.h"
#include "controllers/Controller.h"
#include "controllers/SteamController.h"
#include "controllers/SwitchController.h"
#include "controllers/PS5Controller.h"

std::vector<std::unique_ptr<Controller>> controllers;

struct ParamsStruct{
    const char* midiSong;
    unsigned int intervalUSec;
    int libusbDebugLevel;
    bool repeatSong;
    int reclaimPeriod;
    bool channelMode;
};

float timeElapsedSince(std::chrono::steady_clock::time_point tOrigin){
    using namespace std::chrono;
    steady_clock::time_point tNow = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(tNow - tOrigin);
    return time_span.count();
}

//todo: figure out how to readd this properly
void displayPlayedNotes(int channel, int8_t note) {
//    static int8_t notePerChannel[CHANNEL_COUNT] = {NOTE_STOP, NOTE_STOP};
//    const char* textPerChannel[CHANNEL_COUNT] = {"LEFT haptic : ", ", RIGHT haptic : "};
//    const char* noteBaseNameArray[12] = {" C","C#"," D","D#"," E"," F","F#"," G","G#"," A","A#"," B"};
//
//    if(channel >= CHANNEL_COUNT)
//        return;
//
//    notePerChannel[CHANNEL_COUNT-1-channel] = note;
//
//    for(int i = 0 ; i < CHANNEL_COUNT ; i++){
//        std::cout << textPerChannel[i];
//
//        //Write empty string
//        if(notePerChannel[i] == NOTE_STOP){
//            std::cout << "OFF ";
//        }
//        else{
//            //Write note name
//            std::cout << noteBaseNameArray[notePerChannel[i]%12];
//            int octave = (notePerChannel[i]/12)-1;
//            std::cout << octave;
//            if(octave >= 0 ){
//                std::cout << " ";
//            }
//        }
//    }
//
//    std::cout << "\r" ;
//    std::cout.flush();
}

void playSongPoolMode(const ParamsStruct params) {
    MidiFile_t midifile;

    //Open Midi File
    midifile = MidiFile_load(params.midiSong);

    if (midifile == nullptr) {
        std::cout << "Unable to open song file " << params.midiSong << std::endl;
        return;
    }

    //Check if file contains at least one midi event
    if (MidiFile_getFirstEvent(midifile) == nullptr) {
        std::cout << "Song file " << params.midiSong << " is empty!!" << std::endl;
        return;
    }

    std::cout << "\nStarting playback of " << params.midiSong << std::endl;

    //Get current time point, will be used to know elapsed time
    std::chrono::steady_clock::time_point tOrigin = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point tRestart = std::chrono::steady_clock::now();

    //Iterate through events
    MidiFileEvent_t currentEvent = MidiFile_getFirstEvent(midifile);

    std::vector<std::tuple<Controller*, int, int, int>> controllerChannels; //baby froggie was here
    //Controller, used controller channel num, proper channel num of playing note, note value of playing note (-1 on those 2 if no current playing)
    for (std::unique_ptr<Controller>& controller : controllers) {
        for (int i = 0; i < controller->numChannels(); i++) {
            controllerChannels.emplace_back(controller.get(), i, -1, -1);
        }
    }
    int numChannelsTotal = controllerChannels.size();

    while (currentEvent != nullptr) {
        std::vector<MidiFileEvent_t> eventsToPlay;

        usleep(params.intervalUSec);
        //Every reclaimPeriod seconds, claim the controller to avoid timeouts
        if (timeElapsedSince(tRestart) > params.reclaimPeriod){
            tRestart = std::chrono::steady_clock::now();
            for (std::unique_ptr<Controller>& controller : controllers) {
                controller->reclaim();
            }
        }

        //We now need to play all events with tick < currentTime
        long currentTick = MidiFile_getTickFromTime(midifile,timeElapsedSince(tOrigin));

        //Iterate through all events until the current time, and selecte potential events to play
        for (; currentEvent != nullptr && MidiFileEvent_getTick(currentEvent) < currentTick; currentEvent = MidiFileEvent_getNextEventInFile(currentEvent)) {

            //Only process note start events or note end events matching previous event
            if (!MidiFileEvent_isNoteStartEvent(currentEvent) && !MidiFileEvent_isNoteEndEvent(currentEvent)) continue;

            //Get channel event
            int eventChannel = MidiFileVoiceEvent_getChannel(currentEvent);

            //If channel is outside the number of channels we can play, skip
            if(eventChannel < 0/* || eventChannel >= numChannels*/) continue;

            //If event is note off and does not match previous played event, skip it
            if (MidiFileEvent_isNoteEndEvent(currentEvent)) {
                int channel = MidiFileNoteEndEvent_getChannel(currentEvent);
                int note = MidiFileNoteEndEvent_getNote(currentEvent);
                for (auto& controllerChannel : controllerChannels) {
                    if (std::get<2>(controllerChannel) == channel && std::get<3>(controllerChannel) == note) {
                        std::get<0>(controllerChannel)->playNote(std::get<2>(controllerChannel), NOTE_STOP, DURATION_MAX);
                        std::get<2>(controllerChannel) = -1;
                        std::get<3>(controllerChannel) = -1;
                        break;
                    }
                }
                continue;
            }

            //If we arrive here, this event is accepted
            eventsToPlay.push_back(currentEvent);
            if (eventsToPlay.size() == numChannelsTotal) { //don't bother filling more than can be handled at max
                break;
            }
        }

        for (MidiFileEvent_t event : eventsToPlay) {
            int channel = MidiFileNoteStartEvent_getChannel(event);
            int note = MidiFileNoteStartEvent_getNote(event);
            for (auto& controllerChannel : controllerChannels) {
                if (std::get<2>(controllerChannel) != -1) {
                    continue;
                }
                std::get<0>(controllerChannel)->playNote(std::get<1>(controllerChannel), note, DURATION_MAX);
                std::get<2>(controllerChannel) = channel;
                std::get<3>(controllerChannel) = note;
                break;
            }
        }
    }
    std::cout <<std::endl<< "Playback completed " << std::endl;
}

void playSongChannelMode(const ParamsStruct params){
    MidiFile_t midifile;

    //Open Midi File
    midifile = MidiFile_load(params.midiSong);

    if(midifile == nullptr) {
        std::cout << "Unable to open song file " << params.midiSong << std::endl;
        return;
    }

    //Check if file contains at least one midi event
    if(MidiFile_getFirstEvent(midifile) == nullptr) {
        std::cout << "Song file " << params.midiSong << " is empty!!" << std::endl;
        return;
    }

    std::cout << "\nStarting playback of " << params.midiSong << std::endl;

    int numChannels = 0;
    for (std::unique_ptr<Controller> const& controller : controllers) {
        numChannels += controller->numChannels();
    }
    std::vector<MidiFileEvent_t> acceptedEventPerChannel{static_cast<size_t>(numChannels)};

    //Get current time point, will be used to know elapsed time
    std::chrono::steady_clock::time_point tOrigin = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point tRestart = std::chrono::steady_clock::now();

    //Iterate through events
    MidiFileEvent_t currentEvent = MidiFile_getFirstEvent(midifile);

    std::vector<MidiFileEvent_t> eventsToPlay{static_cast<size_t>(numChannels)};
    while (currentEvent != nullptr) {
        usleep(params.intervalUSec);

        //This will contains the events to play
        std::fill(eventsToPlay.begin(), eventsToPlay.end(), nullptr);

        //We now need to play all events with tick < currentTime
        long currentTick = MidiFile_getTickFromTime(midifile,timeElapsedSince(tOrigin));

        //Every reclaimPeriod seconds, claim the controller to avoid timeouts
        if(timeElapsedSince(tRestart) > params.reclaimPeriod){
            tRestart = std::chrono::steady_clock::now();
            for (std::unique_ptr<Controller>& controller : controllers) {
                controller->reclaim();
            }
        }

        //Iterate through all events until the current time, and selecte potential events to play
        for( ; currentEvent != nullptr && MidiFileEvent_getTick(currentEvent) < currentTick ; currentEvent = MidiFileEvent_getNextEventInFile(currentEvent)){

            //Only process note start events or note end events matching previous event
            if (!MidiFileEvent_isNoteStartEvent(currentEvent) && !MidiFileEvent_isNoteEndEvent(currentEvent)) continue;

            //Get channel event
            int eventChannel = MidiFileVoiceEvent_getChannel(currentEvent);

            //If channel is outside the number of channels we can play, skip
            if(eventChannel < 0 || eventChannel >= numChannels) continue;

            //If event is note off and does not match previous played event, skip it
            if(MidiFileEvent_isNoteEndEvent(currentEvent)){
                MidiFileEvent_t previousEvent = acceptedEventPerChannel[eventChannel];

                //Skip if current event is not ending previous event,
                // or if they share the same tick ( end event after start evetn on same tick )
                if(MidiFileNoteStartEvent_getNote(previousEvent) != MidiFileNoteEndEvent_getNote(currentEvent)
                   ||(MidiFileEvent_getTick(currentEvent) == MidiFileEvent_getTick(previousEvent)))
                    continue;
            }

            //If we arrive here, this event is accepted
            eventsToPlay[eventChannel] = currentEvent;
            acceptedEventPerChannel[eventChannel]=currentEvent;
        }

        size_t controllerIdx = 0;
        std::reference_wrapper<std::unique_ptr<Controller>> controller = controllers[controllerIdx];
        int channelRangeStart = 0;
        //Now play the last events found
        int controllerNumChannels = controller.get()->numChannels();
        for (int currentChannel = 0; currentChannel < numChannels; currentChannel++) {
            if (currentChannel - channelRangeStart >= controllerNumChannels) {
                channelRangeStart = currentChannel;
                controllerIdx++;
                controller = controllers[controllerIdx];
            }

            MidiFileEvent_t selectedEvent = eventsToPlay[currentChannel];

            //If no note event available on the channel, skip it
            if (!MidiFileEvent_isNoteEvent(selectedEvent))
                continue;

            //Set note event
            int8_t eventNote = NOTE_STOP;
            if (MidiFileEvent_isNoteStartEvent(selectedEvent)){
                eventNote = MidiFileNoteStartEvent_getNote(selectedEvent);
            }

            //Play notes
            controller.get()->playNote(currentChannel % controllerNumChannels, eventNote, DURATION_MAX);
            displayPlayedNotes(currentChannel,eventNote);
        }
    }

    std::cout <<std::endl<< "Playback completed " << std::endl;
}

bool parseArguments(int argc, char** argv, ParamsStruct* params){
    int c;
    while ( (c = getopt(argc, argv, "c:l:i:r:o")) != -1) {
        unsigned long int value;
	switch(c){
        case 'c':
	    value = strtoul(optarg,nullptr,10);
            if(value <= 1000000 && value > 0){
                params->reclaimPeriod = value;
            }
            break;
        case 'l':
	    value = strtoul(optarg,nullptr,10);
            if(value >= LIBUSB_LOG_LEVEL_NONE && value <= LIBUSB_LOG_LEVEL_DEBUG){
                params->libusbDebugLevel = value;
            }
            break;
        case 'i':
	    value = strtoul(optarg,nullptr,10);
            if(value <= 1000000 && value > 0){
                params->intervalUSec = value;
            }
            break;
        case 'r':
            params->repeatSong = true;
            break;
        case 'o':
            params->channelMode = true;
            break;
        case '?':
            return false;
            break;
        default:
            break;
        }
    }
    if(optind == argc-1 ){
        params->midiSong = argv[optind];
        return true;
    }
    else{
        return false;
    }
}

void abortPlaying(int) {
    for (std::unique_ptr<Controller>& controller : controllers) {
        controller->abortNote();
        controller->close();
    }

    std::cout << std::endl<< "Aborted " << std::endl;
    std::cout.flush();
    exit(1);
}

int main(int argc, char** argv) {
    std::cout << "Controller Orchestra by A Sickly Silver Moon, Pila, & Roboron3042\n";
    std::cout << "    With components by sarossilli, fossphate, dekuNukem, Div, and the devs of HIDAPI & libusb\n" << std::endl;

    ParamsStruct params;
    params.intervalUSec = DEFAULT_INTERVAL_USEC;
    params.libusbDebugLevel = LIBUSB_LOG_LEVEL_NONE;
    params.repeatSong = false;
    params.midiSong = "\0";
    params.reclaimPeriod = DEFAULT_RECLAIM_PERIOD;
    params.channelMode = false;

    //Parse arguments
    if(!parseArguments(argc, argv, &params)){
        std::cout << "Usage : controllerorchestra [-r][-lDEBUG_LEVEL] [-iINTERVAL] [-cRECLAIM_PERIOD] [-o (channel/old mode)] MIDI_FILE" << std::endl;
        return 1;
    }

    try {
        controllers.push_back(std::make_unique<SteamController>());
    } catch (std::runtime_error& e) {

    }

    SwitchController::openAll(controllers);

//    PS5Controller::openAll(controllers);

    //Set mechanism to stop playing when closing process
    signal(SIGINT, abortPlaying);
    signal(SIGTERM, abortPlaying);

    if (controllers.empty()) {
        throw std::runtime_error{"No controllers found"};
    }

    //Playing song
    do {
        if (params.channelMode) {
            playSongChannelMode(params);
        } else {
            playSongPoolMode(params);
        }
    } while (params.repeatSong);

    for (std::unique_ptr<Controller>& controller : controllers) {
        controller->close();
    }

    return 0;
}
