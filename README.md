# Controller Orchestra

This project is a fork of [Roboron3042's fork of Pila's SteamControllerSinger](https://github.com/Roboron3042/SteamControllerSinger) that has been greatly hacked onto with other libraries and projects to include support for Nintendo Switch controllers.

Projects used include:
* [Roboron3042's SteamControllerSinger](https://github.com/Roboron3042/SteamControllerSinger)
* [sarossilli's Musical-Joycons](https://github.com/sarossilli/Musical-Joycons)
  * Including its components of:
  * [fossphate's JoyCon-Driver](https://github.com/fossephate/JoyCon-Driver)
  * [dekuNukem's Nintendo Switch Reverse Engineering](https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/)
* [HIDAPI](https://github.com/signal11/hidapi) for Switch controllers
* [libusb](https://github.com/libusb/libusb) for Steam controllers
* [Div's MIDI Utilities - midifile](http://www.sreal.com/~div/midi-utilities/) for MIDI parsing

**Note: Controller Orchestra currently only works on GNU/Linux.**

## HOW TO

1. Connect your controllers to your PC
2. Drag the midi file onto controllerorchestra executable
3. Enjoy!

### Where can I find midi songs?

You can find midi songs ready to be played with Steam Controller Singer in my [Personal Collection](https://mega.nz/#F!BWpEWKzB!r7WPw5bZ_domN4pk-FJsjg) (I'll keep updating it with more songs). You can also download midi songs from various websites such [musescore.com](https://musescore.com/), but they may or may not be ready to be played with Steam Controller Singer (see Midi files tips in that case).

### Usage from command prompt:
	controllerorchestra [-r] [-i INTERVAL] [-c RECLAIM_PERIOD] MIDI_FILE

	-i INTERVAL argument to choose player sleep interval (in microseconds). Lower generally means better song fidelity, but higher cpu usage, and at some point going lower won't improve any more. Default value is 10000

	-r to enable repeat mode, which plays continously (restart the song when finished)
	
	-c RECLAIM_PERIOD will change the default reclaiming period (2 seconds). A greater period will reduce notes hanging, but will also increase the probability of losing the control of the controller, depending on the song. All songs are known to fail with claim period over 5. After a failed attempt, you will need to reconnect the controller.

### Midi files tips:

Midi files may need to be edited with a software such [MidiEditor](https://www.midieditor.org/) to be correctly played with Controller Orchestra following the next tips:

* Channels are assigned to devices based on the order the program finds the controllers.
  * It will load 1 Steam controller first with 2 channels (if possible), then as many Switch controllers with 1 channel as possible.
* So with 1 Steam controller and 2 pro controllers, channels 1 and 2 are Steam controller, 3 and 4 are Pro controllers.
* **Having multiple notes at once on a channel will only play one of them.**

## Compiling

It uses CMake, its only been tested on a Manjaro device with relevant libs installed already. But it should work on any Linux distro with the correct libs installed.
