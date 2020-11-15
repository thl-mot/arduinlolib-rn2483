/*
 * rn2483.cpp
 *
 *  Created on: 14.11.2020
 *      Author: thomas
 */

#include "rn2483.h"

#define TIMEOUT_DEFAULT (2000)
#define TIMEOUT_SAVE (30000)
#define TIMEOUT_TX (10000)
#define TIMEOUT_JOIN (30000)

#define BAUD_RATE 57600
#define BAUD_BREAK 40000

rn2483::rn2483(HardwareSerial &serial, int resetPin) :
		_rn2483Serial(serial), _resetPin(resetPin) {
	_rn2483Serial.setTimeout(TIMEOUT_DEFAULT);
	_rn2483Serial.begin(57600);
	_rn2483Serial.flush();
}

bool rn2483::initHardware() {
	//reset RN2xx3
	pinMode(_resetPin, OUTPUT);
	digitalWrite(_resetPin, LOW);
	delay(100);
	digitalWrite(_resetPin, HIGH);

	delay(100); //wait for the RN2xx3's startup message
	_rn2483Serial.flush();

	int retries = 10;
	String version = sendRaw(F("sys get ver"));
	while (!version.startsWith(F("RN2483")) && retries > 0) {
		delay(1000);
		version = sendRaw(F("sys get ver"));
		retries--;
	}

	return retries > 0;
}

bool rn2483::prepareABP(const String &addr, const String &appSKey,
		const String &nwkSKey) {
	checkSleepBeforeCommand();
	return false;
}

bool rn2483::prepareOTAA(const String &devEUI = "", const String &appEUI = "",
		const String &appKey = "") {
	checkSleepBeforeCommand();
	clearSerial();

	sendRaw(F("mac reset 868"));

	sendRaw(F("mac reset 868"));

	if (devEUI.length() == 16) {
		_devEUI = devEUI;
	} else {
		_devEUI = sendRaw(F("sys get hweui"));
	}

	sendMacSet(F("deveui"), _devEUI);

	_appEUI = appEUI;
	sendMacSet(F("appeui"), _appEUI);

	_appKey = appKey;
	sendMacSet(F("appkey"), _appKey);

	sendMacSet(F("pwridx"), "1");

	initFrequencyPlan();

	// adaptive datarate

	// auomatic reply

	sendMacSave();
	return true;
}

bool rn2483::join() {
	checkSleepBeforeCommand();
	_rn2483Serial.setTimeout(TIMEOUT_JOIN);

	_joined = false;

	// Only try twice to join, then return and let the user handle it.
	for (int i = 0; i < 2 && !_joined; i++) {
		sendRaw(F("mac join otaa"));
		// Parse 2nd response
		String receivedData = _rn2483Serial.readStringUntil('\n');

		if (receivedData.startsWith(F("accepted"))) {
			_joined = true;
			delay(1000);
		} else {
			delay(1000);
		}
	}
	_rn2483Serial.setTimeout(TIMEOUT_DEFAULT);
	return _joined;
}

bool rn2483::sleep(long ms) {
	clearSerial();
	String cmd = F("sys sleep ");
	cmd += String(ms);
	_rn2483Serial.println(cmd);
	_sleeping = true;
	return true;
}

bool rn2483::wakeUp() {
	bool again = true;

	// Try a maximum of 10 times with a 1 second delay
	for (uint8_t i = 0; i < 10 && again; i++) {
		delay(1000);
		_rn2483Serial.end();
		_rn2483Serial.begin(BAUD_BREAK);
		_rn2483Serial.write((byte) 0x00);
		_rn2483Serial.flush();
		_rn2483Serial.end();
		_rn2483Serial.begin(BAUD_RATE);

		_rn2483Serial.write(0x55);
		_rn2483Serial.println();

		// we could use sendRawCommand(F("sys get ver")); here
		again = sendRawExpectOk("sys get ver");
	}
	_sleeping = false;
	return again;
}

bool rn2483::poll() {
	return false;
}

bool rn2483::tx(String hexData, bool confirmed) {
	checkSleepBeforeCommand();
	clearSerial();

	_rn2483Serial.setTimeout( TIMEOUT_TX);

	String cmd = F("mac tx");
	if (confirmed) {
		cmd += F(" cnf");
	} else {
		cmd += F(" uncnf");
	}
	cmd += " 1 ";
	cmd += hexData;

	String response = sendRaw(cmd);

	bool ok = false;
	if (response.equals(F("ok"))) {
		ok = true;
	} else if (response.equals(F("invalid_param"))) {
		ok = false;
	} else if (response.equals(F("not_joined"))) {
		ok = false;
	} else if (response.equals(F("no_free_ch"))) {
		ok = false;
	} else if (response.equals(F("silent"))) {
		ok = false;
	} else if (response.equals(F("frame_counter_err_rejoin_needed"))) {
		ok = false;
	} else if (response.equals(F("busy"))) {
		ok = false;
	} else if (response.equals(F("mac_paused"))) {
		ok = false;
	} else if (response.equals(F("invalid_data_len"))) {
		ok = false;
	}

	if (ok) {
		while (true) {

			// there will be a second transmission
			response = _rn2483Serial.readStringUntil('\n');
			Serial.println(" -> " + response);

			if (response.equals(F("mac_tx_ok"))) {

			} else if (response.startsWith(F("mac_rx"))) {
				break;
			} else if (response.equals(F("mac_err"))) {
				break;
			} else if (response.equals(F("invalid_data_len"))) {
				break;
			} else {
				// unknown response
				ok = false;
				break;
			}
		}

	}

	_rn2483Serial.setTimeout(TIMEOUT_DEFAULT);
	return ok;
}

bool rn2483::isJoined() {
	return _joined;
}

bool rn2483::sendMacSave() {
	checkSleepBeforeCommand();
	_rn2483Serial.setTimeout(TIMEOUT_SAVE);
	bool ok = sendRawExpectOk(F("mac save"));
	_rn2483Serial.setTimeout(TIMEOUT_DEFAULT);
	return ok;
}

bool rn2483::sendMacSet(const String &param, const String &value) {
	checkSleepBeforeCommand();
	String command;
	command.reserve(10 + param.length() + value.length());
	command = F("mac set ");
	command += param;
	command += ' ';
	command += value;
	return sendRawExpectOk(command);
}

String rn2483::sendMacGet(const String &param) {
	String command;
	command.reserve(10 + param.length());
	command = F("mac get ");
	command += param;
	return sendRaw(command);
}

String rn2483::sendMacGetCh(const String &param, unsigned int channel) {
	String command;
	command.reserve(20);
	command = F("ch ");
	command += param;
	command += ' ';
	command += channel;
	return sendMacGet(command);
}

bool rn2483::sendSetChannelDutyCycle(unsigned int channel,
		unsigned int dutyCycle) {
	return sendMacSetCh(F("dcycle"), channel, dutyCycle);
}

bool rn2483::sendSetChannelFrequency(unsigned int channel, uint32_t frequency) {
	return sendMacSetCh(F("freq"), channel, frequency);
}

bool rn2483::sendSetChannelEnabled(unsigned int channel, bool enabled) {
	return sendMacSetCh(F("status"), channel, enabled ? F("on") : F("off"));
}

bool rn2483::sendSetChannelDataRateRange(unsigned int channel,
		unsigned int minRange, unsigned int maxRange) {
	String value;
	value = String(minRange);
	value += ' ';
	value += String(maxRange);
	return sendMacSetCh(F("drrange"), channel, value);
}

bool rn2483::sendSet2ndRecvWindow(unsigned int dataRate, uint32_t frequency) {
	String value;
	value = String(dataRate);
	value += ' ';
	value += String(frequency);
	return sendMacSet(F("rx2"), value);
}

bool rn2483::sendMacSetCh(const String &param, unsigned int channel,
		const String &value) {
	String command;
	command.reserve(20);
	command = param;
	command += ' ';
	command += channel;
	command += ' ';
	command += value;
	return sendMacSet(F("ch"), command);
}

bool rn2483::sendMacSetCh(const String &param, unsigned int channel,
		uint32_t value) {
	return sendMacSetCh(param, channel, String(value));
}

bool rn2483::initFrequencyPlan() {
	bool returnValue;

	switch (_frequencyPlan) {
	case SINGLE_CHANNEL_EU: {
		//mac set rx2 <dataRate> <frequency>
		//set2ndRecvWindow(5, 868100000); //use this for "strict" one channel gateways
		sendSet2ndRecvWindow(3, 869525000); //use for "non-strict" one channel gateways
		sendSetChannelDutyCycle(0, 99); //1% duty cycle for this channel
		sendSetChannelDutyCycle(1, 65535); //almost never use this channel
		sendSetChannelDutyCycle(2, 65535); //almost never use this channel
		for (uint8_t ch = 3; ch < 8; ch++) {
			sendSetChannelEnabled(ch, false);
		}
		returnValue = true;
		break;
	}

	case TTN_EU: {
		/*
		 * The <dutyCycle> value that needs to be configured can be
		 * obtained from the actual duty cycle X (in percentage)
		 * using the following formula: <dutyCycle> = (100/X) – 1
		 *
		 *  10% -> 9
		 *  1% -> 99
		 *  0.33% -> 299
		 *  8 channels, total of 1% duty cycle:
		 *  0.125% per channel -> 799
		 *
		 * Most of the TTN_EU frequency plan was copied from:
		 * https://github.com/TheThingsNetwork/arduino-device-lib
		 */

		uint32_t freq = 867100000;
		for (uint8_t ch = 0; ch < 8; ch++) {
			sendSetChannelDutyCycle(ch, 799); // All channels
			if (ch == 1) {
				sendSetChannelDataRateRange(ch, 0, 6);
			} else if (ch > 2) {
				sendSetChannelDataRateRange(ch, 0, 5);
				sendSetChannelFrequency(ch, freq);
				freq = freq + 200000;
			}
			sendSetChannelEnabled(ch, true); // frequency, data rate and duty cycle must be set first.
		}

		//RX window 2
		sendSet2ndRecvWindow(3, 869525000);

		returnValue = true;
		break;
	}

	case TTN_US: {
		returnValue = false;
		break;
	}

	case DEFAULT_EU: {
		for (int channel = 0; channel < 3; channel++) {
			//fix duty cycle - 1% = 0.33% per channel
			sendSetChannelDutyCycle(channel, 799);
			sendSetChannelEnabled(channel, true);
		}
		for (int channel = 3; channel < 8; channel++) {
			sendSetChannelEnabled(channel, false);
		}
		returnValue = true;
		break;
	}
	default: {
		//set default channels 868.1, 868.3 and 868.5?
		returnValue = false; //well we didn't do anything, so yes, false
		break;
	}
	}

	return returnValue;

}

void rn2483::clearSerial() {
	while (_rn2483Serial.available()) {
		_rn2483Serial.read();
	}
}

bool rn2483::sendRawExpectOk(const String &cmd) {
	String r = sendRaw(cmd);
	return r.equals(F("ok"));
}

String rn2483::sendRaw(const String &cmd) {
	clearSerial();
	_rn2483Serial.println(cmd);

	String response = _rn2483Serial.readStringUntil('\n');
	response.trim();
	if (response.equals(F("invalid_param"))) {
		Serial.println("CMD: '" + cmd + "' -> " + response);
	}
	return response;
}

void rn2483::checkSleepBeforeCommand() {
	if (_sleeping) {
		wakeUp();
	}
}

void rn2483::printSettings(Stream &stream) {
	checkSleepBeforeCommand();
	clearSerial();
	stream.println(F("*** Settings ***"));
	stream.println(sendRaw(F("sys get ver")));
	stream.print(F("hweui "));
	stream.println(sendRaw(F("sys get hweui")));
	stream.print(F("deveui "));
	stream.println(sendRaw(F("mac get deveui")));
	stream.print(F("appeui "));
	stream.println(sendRaw(F("mac get appeui")));
	stream.print(F("appkey "));
	stream.println(F("*"));

	for (int i = 0; i < 15; i++) {
		stream.print("Ch" + String(i));
		stream.print(" dcycle ");
		stream.print(sendMacGetCh(F("dcycle"), i));
		stream.print(" freq ");
		stream.print(sendMacGetCh(F("freq"), i));
		stream.print(" drrange ");
		stream.print(sendMacGetCh(F("drrange"), i));
		stream.print(" status ");
		stream.print(sendMacGetCh(F("status"), i));
		stream.println();
	}
	stream.println("***************");
}

bool rn2483::setNvm(int addr, char value) {
	checkSleepBeforeCommand();
	String cmd;
	cmd.reserve(18);
	cmd = F("sys set nvm ");
	cmd += String(addr, HEX);
	cmd += ' ';
	cmd += String(value, HEX);
	return sendRawExpectOk(cmd);
}
char rn2483::getNvm(int addr) {
	checkSleepBeforeCommand();
	String cmd;
	cmd.reserve(14);
	cmd = F("sys get nvm ");
	cmd += String(addr, HEX);
	String str = sendRaw(cmd);
	char v = 0;
	if (str.length() > 0) {
		char c = str.charAt(0);
		v = hex2byte(c);
	}
	if (str.length() > 1) {
		v = v * 16 + hex2byte(str.charAt(1));
	}
	return v;
}

bool rn2483::setPinmode(String pin, String mode) {
	checkSleepBeforeCommand();
	String cmd;
	cmd.reserve(18);
	cmd = F("sys set pinmode ");
	cmd += pin;
	cmd += ' ';
	cmd += mode;
	return sendRawExpectOk(cmd);
}

String rn2483::getPinmode(String pin) {
	checkSleepBeforeCommand();
	String cmd;
	cmd.reserve(18);
	cmd = F("sys get pinmode ");
	cmd += pin;
	return sendRaw(cmd);
}

bool rn2483::setPin(String pin, bool state) {
	checkSleepBeforeCommand();
	String cmd;
	cmd.reserve(18);
	cmd = F("sys set pindig ");
	cmd += pin;
	cmd += ' ';
	cmd += String(state);
	return sendRawExpectOk(cmd);
}
bool rn2483::getPin(String pin) {
	checkSleepBeforeCommand();
	String cmd;
	cmd.reserve(18);
	cmd = F("sys get pindig ");
	cmd += pin;
	return sendRaw(cmd).equals("1");
}

int rn2483::getAnalog(String pin) {
	checkSleepBeforeCommand();
	String cmd;
	cmd.reserve(18);
	cmd = F("sys get pinana ");
	cmd += pin;
	String str = sendRaw(cmd);
	return atoi(str.c_str());
}

int rn2483::getVDD() {
	checkSleepBeforeCommand();
	String str = sendRaw(F("sys get vdd")) + "";
	Serial.println(str);
	return str.toInt();
}

char rn2483::hex2byte(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	} else if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	return 0;
}
