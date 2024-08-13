#include <bitset>
#include <hidapi.h>
#include <cstring>
#include <cmath>
#include <cstdint>

using namespace std;
#define JOYCON_VENDOR 0x057e
#define JOYCON_L_BT 0x2006
#define JOYCON_R_BT 0x2007
#define PRO_CONTROLLER 0x2009
#define JOYCON_CHARGING_GRIP 0x200e

#define INTENSITY_VAL 0x02

class Joycon {

public:

	hid_device *handle;
	wchar_t *serial;

	std::string name;

	int deviceNumber = 0;// left(0) or right(1) vjoy

	bool bluetooth = true;

    enum class TYPE : int {
        TYPE_NONE = 0,
        TYPE_LEFT_JOYCON,
        TYPE_RIGHT_JOYCON,
        TYPE_PRO_CONTROLLER,
    };
	TYPE control_type = TYPE::TYPE_NONE; // 1: left joycon, 2: right joycon, 3: pro controller

	uint8_t battery;

	int global_count = 0;

	struct brcm_hdr {
		uint8_t cmd;
		uint8_t rumble[9];
	};


	struct brcm_cmd_01 {
		uint8_t subcmd;
		uint32_t offset;
		uint8_t size;
	};

	int timing_byte = 0x0;


public:

	Joycon(struct hid_device_info *dev) {

		if (dev->product_id == JOYCON_CHARGING_GRIP) {
			if (dev->interface_number == 0 || dev->interface_number == -1) {
				this->name = std::string("Joy-Con (R)");
				this->control_type = TYPE::TYPE_RIGHT_JOYCON; // right joycon
			} else if (dev->interface_number == 1) {
				this->name = std::string("Joy-Con (L)");
				this->control_type = TYPE::TYPE_LEFT_JOYCON; // left joycon
			}
		}

		if (dev->product_id == JOYCON_L_BT) {
			this->name = std::string("Joy-Con (L)");
			this->control_type = TYPE::TYPE_LEFT_JOYCON; // left joycon
		} else if (dev->product_id == JOYCON_R_BT) {
			this->name = std::string("Joy-Con (R)");
			this->control_type = TYPE::TYPE_RIGHT_JOYCON; // right joycon
		} else if (dev->product_id == PRO_CONTROLLER) {
			this->name = std::string("Pro Controller");
			this->control_type = TYPE::TYPE_PRO_CONTROLLER;
		}

		this->serial = wcsdup(dev->serial_number);

		//printf("Found joycon %c %i: %ls %s\n", L_OR_R(this->control_type), joycons.size(), this->serial, dev->path);
		printf("Found joycon: %ls %s\n", this->serial, dev->path);
		this->handle = hid_open_path(dev->path);


		if (this->handle == nullptr) {
			printf("Could not open serial %ls: %s\n", this->serial, strerror(errno));
			throw;
		}
	}

	void hid_exchange(hid_device *handle, unsigned char *buf, int len) {
		if (!handle) return;

		int res;

		res = hid_write(handle, buf, len);

		//if (res < 0) {
		//	printf("Number of bytes written was < 0!\n");
		//} else {
		//	printf("%d bytes written.\n", res);
		//}

		//// set non-blocking:./
		//hid_set_nonblocking(handle, 1);

		res = hid_read(handle, buf, 0x40);

		//if (res < 1) {
		//	printf("Number of bytes read was < 1!\n");
		//} else {
		//	printf("%d bytes read.\n", res);
		//}
	}


	void send_command(int command, uint8_t *data, int len) {
		unsigned char buf[0x40];
		memset(buf, 0, 0x40);

		if (!bluetooth) {
			buf[0x00] = 0x80;
			buf[0x01] = 0x92;
			buf[0x03] = 0x31;
		}

		buf[bluetooth ? 0x0 : 0x8] = command;
		if (data != nullptr && len != 0) {
			memcpy(buf + (bluetooth ? 0x1 : 0x9), data, len);
		}

		hid_exchange(this->handle, buf, len + (bluetooth ? 0x1 : 0x9));

		if (data) {
			memcpy(data, buf, 0x40);
		}
	}

	void send_subcommand(int command, int subcommand, uint8_t *data, int len) {
		unsigned char buf[0x40];
		memset(buf, 0, 0x40);

		int rumble_base[9] = { (++global_count) & 0xF, 0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40 };
		memcpy(buf, rumble_base, 9);

		if (global_count > 0xF) {
			global_count = 0x0;
		}

		// set neutral rumble base only if the command is vibrate (0x01)
		// if set when other commands are set, might cause the command to be misread and not executed
		//if (subcommand == 0x01) {
		//	uint8_t rumble_base[9] = { (++global_count) & 0xF, 0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40 };
		//	memcpy(buf + 10, rumble_base, 9);
		//}

		buf[9] = subcommand;
		if (data && len != 0) {
			memcpy(buf + 10, data, len);
		}

		send_command(command, buf, 10 + len);

		if (data) {
			memcpy(data, buf, 0x40); //TODO
		}
	}

	void rumble(int frequency, int intensity) {

		unsigned char buf[0x400];
		memset(buf, 0, 0x40);

		// intensity: (0, 8)
		// frequency: (0, 255)

		//	 X	AA	BB	 Y	CC	DD
		//[0 1 x40 x40 0 1 x40 x40] is neutral.


		//for (int j = 0; j <= 8; j++) {
		//	buf[1 + intensity] = 0x1;//(i + j) & 0xFF;
		//}

        if (this->control_type == TYPE::TYPE_LEFT_JOYCON) {
            rumble_l(frequency, intensity);
        } else if (this->control_type == TYPE::TYPE_RIGHT_JOYCON) {
            rumble_r(frequency, intensity);
        } else { //pro-controller
            rumble_l(frequency, intensity);
            rumble_r(frequency, intensity);
        }
	}

    void rumble_whatever() {
        uint8_t buff[0x400];
        memset(buff, 0, 0x40);

        buff[0] = 0x10;

        uint8_t* buf = buff + 1;
//        buf[2] = 0x7f; //626 Hz
//        buf[3] = 0x72; //1.0 Amplitude

        buf[0] = 0x00;
        buf[1] = 0xC8 | 0x01;
        buf[2] = 0x60;
//        buf[3] = 0x72;

        send_command(0x10, buff, 9);
    }

    void rumble_l(int frequency, int intensity) {
        uint8_t buf[0x400];
        memset(buf, 0, 0x40);

        // intensity: (0, 8)
        // frequency: (0, 255)

        buf[1 + 0 + intensity] = INTENSITY_VAL;

        buf[1 + 0] = frequency;

        // set non-blocking:
        hid_set_nonblocking(this->handle, 1);

        send_command(0x10, buf, 13);
    }

    void rumble_r(int frequency, int intensity) {
        uint8_t buf[0x400];
        memset(buf, 0, 0x40);

        // intensity: (0, 8)
        // frequency: (0, 255)

        buf[1 + 4 + intensity] = INTENSITY_VAL;

        buf[1 + 4] = frequency;

        // set non-blocking:
        hid_set_nonblocking(this->handle, 1);

        send_command(0x10, buf, 13);
    }

	void rumble2(uint16_t hf, uint8_t hfa, uint8_t lf, uint16_t lfa) {
		unsigned char buf[0x400];
		memset(buf, 0, 0x40);


		//int hf		= HF;
		//int hf_amp	= HFA;
		//int lf		= LF;
		//int lf_amp	= LFA;
		// maybe:
		//int hf_band = hf + hf_amp;

		int off = 0;// offset
		if (this->control_type == TYPE::TYPE_RIGHT_JOYCON) {
			off = 4;
		}


		// Byte swapping
		buf[0 + off] = hf & 0xFF;
		buf[1 + off] = hfa + ((hf >> 8) & 0xFF); //Add amp + 1st byte of frequency to amplitude byte

												 // Byte swapping
		buf[2 + off] = lf + ((lfa >> 8) & 0xFF); //Add freq + 1st byte of LF amplitude to the frequency byte
		buf[3 + off] = lfa & 0xFF;


		// set non-blocking:
		hid_set_nonblocking(this->handle, 1);

		send_command(0x10, (uint8_t*)buf, 0x9);
	}

	void rumble3(float frequency, uint8_t hfa, uint16_t lfa) {

		//Float frequency to hex conversion
		if (frequency < 0.0f) {
			frequency = 0.0f;
		} else if (frequency > 1252.0f) {
			frequency = 1252.0f;
		}
		uint8_t encoded_hex_freq = (uint8_t)round(log2((double)frequency / 10.0)*32.0);

		//uint16_t encoded_hex_freq = (uint16_t)floor(-32 * (0.693147f - log(frequency / 5)) / 0.693147f + 0.5f); // old

		//Convert to Joy-Con HF range. Range in big-endian: 0x0004-0x01FC with +0x0004 steps.
		uint16_t hf = (encoded_hex_freq - 0x60) * 4;
		//Convert to Joy-Con LF range. Range: 0x01-0x7F.
		uint8_t lf = encoded_hex_freq - 0x40;

		rumble2(hf, hfa, lf, lfa);
	}



	void rumble_freq(uint16_t hf, uint8_t hfa, uint8_t lf, uint16_t lfa) {
		unsigned char buf[0x400];
		memset(buf, 0, 0x40);

		//int hf		= HF;
		//int hf_amp	= HFA;
		//int lf		= LF;
		//int lf_amp	= LFA;
		// maybe:
		//int hf_band = hf + hf_amp;

		int off = 0;// offset
		if (this->control_type == TYPE::TYPE_RIGHT_JOYCON) {
			off = 4;
		}


		// Byte swapping
		buf[0 + off] = hf & 0xFF;
		buf[1 + off] = hfa + ((hf >> 8) & 0xFF); //Add amp + 1st byte of frequency to amplitude byte

												 // Byte swapping
		buf[2 + off] = lf + ((lfa >> 8) & 0xFF); //Add freq + 1st byte of LF amplitude to the frequency byte
		buf[3 + off] = lfa & 0xFF;


		// set non-blocking:
		hid_set_nonblocking(this->handle, 1);

		send_command(0x10, (uint8_t*)buf, 0x9);
	}

    void rumble_f(double freq) {
        unsigned char byte[0x400];
        memset(byte, 0, 0x40);

        //Float frequency to hex conversion
        if (freq < 0.0f)
            freq = 0.0f;
        else if (freq > 1252.0f)
            freq = 1252.0f;
        uint8_t encoded_hex_freq = (uint8_t)round(log2((double)freq/10.0)*32.0);

        //Convert to Joy-Con HF range. Range in big-endian: 0x0004-0x01FC with +0x0004 steps.
        uint16_t hf = (encoded_hex_freq-0x60)*4;
        //Convert to Joy-Con LF range. Range: 0x01-0x7F.
        uint8_t lf = encoded_hex_freq-0x40;

        // Float amplitude to hex conversion
        uint8_t encoded_hex_amp = 0;
        float amp = 0.7f;
        if(amp > 0.23f)
            encoded_hex_amp = (uint8_t)round(log2f(amp*8.7f)*32.f);
        else if(amp > 0.12f)
            encoded_hex_amp = (uint8_t)round(log2f(amp*17.f)*16.f);
        else{
            // TBD
        }
        uint16_t hf_amp = encoded_hex_amp * 2;    // encoded_hex_amp<<1;
        uint8_t lf_amp = encoded_hex_amp / 2 + 64;// (encoded_hex_amp>>1)+0x40;

        int off = 0;// offset
        if (this->control_type == TYPE::TYPE_RIGHT_JOYCON) {
            off = 4;
        }

        //Left linear actuator
//        uint16_t hf = 0x01a8; //Set H.Frequency
//        uint8_t hf_amp = 0x88; //Set H.Frequency amplitude
        //Byte swapping
        byte[0 + off] = hf & 0xFF;
        byte[1 + off] = hf_amp + ((hf >> 8) & 0xFF); //Add amp + 1st byte of frequency to amplitude byte

//        uint8_t lf = 0x63; //Set L.Frequency
//        uint16_t lf_amp = 0x804d; //Set L.Frequency amplitude
        //Byte swapping
        byte[2 + off] = lf + ((lf_amp >> 8) & 0xFF); //Add freq + 1st byte of LF amplitude to the frequency byte
        byte[3 + off] = lf_amp & 0xFF;

        // set non-blocking:
        hid_set_nonblocking(this->handle, 1);

        send_command(0x10, (uint8_t*)byte, 0x9);
    }

	int init_bt() {

		this->bluetooth = true;

		unsigned char buf[0x40];
		memset(buf, 0, 0x40);


		// set blocking to ensure command is recieved:
		hid_set_nonblocking(this->handle, 0);

		// Enable vibration
		printf("Enabling vibration...\n");
		buf[0] = 0x01; // Enabled
		send_subcommand(0x1, 0x48, buf, 1);

		// Enable IMU data
		printf("Enabling IMU data...\n");
		buf[0] = 0x01; // Enabled
		send_subcommand(0x01, 0x40, buf, 1);


		// Set input report mode (to push at 60hz)
		// x00	Active polling mode for IR camera data. Answers with more than 300 bytes ID 31 packet
		// x01	Active polling mode
		// x02	Active polling mode for IR camera data.Special IR mode or before configuring it ?
		// x21	Unknown.An input report with this ID has pairing or mcu data or serial flash data or device info
		// x23	MCU update input report ?
		// 30	NPad standard mode. Pushes current state @60Hz. Default in SDK if arg is not in the list
		// 31	NFC mode. Pushes large packets @60Hz
		printf("Set input report mode to 0x30...\n");
		buf[0] = 0x30;
		send_subcommand(0x01, 0x03, buf, 1);

		// @CTCaer

		printf("Successfully initialized %s!\n", this->name.c_str());

		return 0;
	}

    

    // SPI (@CTCaer):

	int get_spi_data(uint32_t offset, const uint16_t read_len, uint8_t *test_buf) {
		int res;
		uint8_t buf[0x100];
		while (1) {
			memset(buf, 0, sizeof(buf));
			auto hdr = (brcm_hdr *)buf;
			auto pkt = (brcm_cmd_01 *)(hdr + 1);
			hdr->cmd = 1;
			hdr->rumble[0] = timing_byte;

			buf[1] = timing_byte;

			timing_byte++;
			if (timing_byte > 0xF) {
				timing_byte = 0x0;
			}
			pkt->subcmd = 0x10;
			pkt->offset = offset;
			pkt->size = read_len;

			for (int i = 11; i < 22; ++i) {
				buf[i] = buf[i+3];
			}

			res = hid_write(handle, buf, sizeof(*hdr) + sizeof(*pkt));

			res = hid_read(handle, buf, sizeof(buf));

			if ((*(uint16_t*)&buf[0xD] == 0x1090) && (*(uint32_t*)&buf[0xF] == offset)) {
				break;
			}
		}
		if (res >= 0x14 + read_len) {
			for (int i = 0; i < read_len; i++) {
				test_buf[i] = buf[0x14 + i];
			}
		}

		return 0;
	}

	int write_spi_data(uint32_t offset, const uint16_t write_len, uint8_t* test_buf) {
		int res;
		uint8_t buf[0x100];
		int error_writing = 0;
		while (1) {
			memset(buf, 0, sizeof(buf));
			auto hdr = (brcm_hdr *)buf;
			auto pkt = (brcm_cmd_01 *)(hdr + 1);
			hdr->cmd = 1;
			hdr->rumble[0] = timing_byte;
			timing_byte++;
			if (timing_byte > 0xF) {
				timing_byte = 0x0;
			}
			pkt->subcmd = 0x11;
			pkt->offset = offset;
			pkt->size = write_len;
			for (int i = 0; i < write_len; i++) {
				buf[0x10 + i] = test_buf[i];
			}
			res = hid_write(handle, buf, sizeof(*hdr) + sizeof(*pkt) + write_len);

			res = hid_read(handle, buf, sizeof(buf));

			if (*(uint16_t*)&buf[0xD] == 0x1180)
				break;

			error_writing++;
			if (error_writing == 125) {
				return 1;
			}
		}

		return 0;

	}
};