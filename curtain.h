#include "esphome.h"

// basic commands
#define TUYA_CLOSE   "55aa000600056504000100"
#define TUYA_STOP    "55aa000600056504000101"
#define TUYA_OPEN    "55aa000600056504000102"

// set position percentage
#define TUYA_SET_POSITION  "55aa0006000868020004000000" // and 1 byte (0x00-0x64)

// enable/disable reversed motor direction
#define TUYA_DISABLE_REVERSING  "55aa000600056700000100"
#define TUYA_ENABLE_REVERSING  "55aa000600056700000101"

class CustomCurtain : public Component, public Cover {
public:
	CoverTraits get_traits() override {
		auto traits = CoverTraits();
		traits.set_is_assumed_state(false);
		traits.set_supports_position(true);
		traits.set_supports_tilt(false);
		return traits;
	}

	void setup() override {
		position = id(cover_position);
		publish_state();
	}

	void loop() override {
		while (Serial.available()) {
			readByte(Serial.read());
		}
	}

	void control(const CoverCall &call) override {
		if (call.get_stop()) {
			crc = 0;
			writeHex(TUYA_STOP);
			writeByte(crc);
		}
		if (call.get_position().has_value()) {
			crc = 0;
			uint8_t pos = *call.get_position() * 100.0f;
			if (pos == 100) {
				writeHex(TUYA_OPEN);
			} else if (pos == 0) {
				writeHex(TUYA_CLOSE);
			} else {
				writeHex(TUYA_SET_POSITION);
				writeByte(pos);
			}
			writeByte(crc);
		}
	}

	void readByte(uint8_t data) {
		if ((offset == 0 && data == 0x55) || (offset == 1 && data == 0xAA) || offset) {
			buffer[offset++] = data;
			if (offset > 6) {
				int len = buffer[4] << 8 | buffer[5];
				if (7 + len == offset) {
					uint8_t crc = buffer[offset - 1];
					uint8_t sum = 0;
					for (int i = 0; i < offset - 1; i++) sum += buffer[i];
					if (sum == crc) {
						uint8_t cmd_type = buffer[3];
						uint8_t data_id = buffer[6];
						uint8_t data_size = buffer[9]; // only 1byte or 4bytes values
						switch (cmd_type) {
							case 0x07: {
								switch (data_id) {
									// Motor reversing state
									case 0x67: {
										id(cover_reversed) = !!buffer[10];
										break;
									}
									// Operation mode state
									case 0x65: {
										switch (buffer[10]) {
											case 0x00:
												current_operation = COVER_OPERATION_CLOSING;
												break;
											case 0x01:
												current_operation = COVER_OPERATION_IDLE;
												break;
											case 0x02:
												current_operation = COVER_OPERATION_OPENING;
												break;
										}
										break;
									}
									// Position report during operation
									// Max value is 0x64 so the last byte is good enough
									case 0x66: {
										position = buffer[13] / 100.0f;
										id(cover_open) = !!position;
										break;
									}
									// Position report after operation
									case 0x68: {
										position = buffer[13] / 100.0f;
										if (position > 0.95) position = 1;
										if (position < 0.05) position = 0;
										id(cover_open) = !!position;
										current_operation = COVER_OPERATION_IDLE;
										break;
									}
								}
								publish_state();
								break;
							}
						}
					}
					offset = 0;
				}
			}
		}
	}

	uint8_t crc;

	void writeByte(uint8_t data) {
		Serial.write(data);
		crc += data;
	}

	void writeHex(std::string data) {
		int i = 0;
		while (i < data.length()) {
			const char hex[2] = {data[i++], data[i++]};
			uint8_t d = strtoul(hex, NULL, 16);
			writeByte(d);
		}
	}

protected:
	uint16_t offset = 0;
	uint8_t buffer[1024];
};

class CustomAPI : public Component, public CustomAPIDevice {
public:
	void setup() override {
		register_service(&CustomAPI::setMotorNormal, "set_motor_normal");
		register_service(&CustomAPI::setMotorReversed, "set_motor_reversed");
		register_service(&CustomAPI::sendMessage, "send_command", {"data"});
	}

	void setMotorNormal() {
		sendMessage(TUYA_DISABLE_REVERSING);
	}

	void setMotorReversed() {
		sendMessage(TUYA_ENABLE_REVERSING);
	}

	void sendMessage(std::string data) {
		int i = 0;
		uint8_t sum = 0;
		while (i < data.length()) {
			const char hex[2] = {data[i++], data[i++]};
			uint8_t d = strtoul(hex, NULL, 16);
			sum += d;
			writeByte(d);
		}
		writeByte(sum);
	}

	void writeByte(uint8_t data) {
		Serial.write(data);
	}
};
