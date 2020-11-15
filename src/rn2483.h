/*
 * rn2483.h
 *
 *  Created on: 14.11.2020
 *      Author: thomas
 */

#ifndef RN2483_H_
#define RN2483_H_

#include "Arduino.h"
#include "HardwareSerial.h"

enum FREQ_PLAN {
	SINGLE_CHANNEL_EU, TTN_EU, TTN_US, DEFAULT_EU
};

#define	RN2483_DIGITAL_IN "digin"
#define	RN2483_DIGITAL_OUT "digout"
#define	RN2483_ANALOG_IN "ana"

#define	RN2483_GPIO0 "GPIO0"
#define	RN2483_GPIO1 "GPIO1"
#define	RN2483_GPIO2 "GPIO2"
#define	RN2483_GPIO3 "GPIO3"
#define	RN2483_GPIO4 "GPIO4"
#define	RN2483_GPIO5 "GPIO5"
#define	RN2483_GPIO6 "GPIO6"
#define	RN2483_GPIO7 "GPIO7"
#define	RN2483_GPIO8 "GPIO8"
#define	RN2483_GPIO9 "GPIO9"
#define	RN2483_GPIO10 "GPIO10"
#define	RN2483_GPIO11 "GPIO11"
#define	RN2483_GPIO12 "GPIO12"
#define	RN2483_GPIO13 "GPIO13"
#define	RN2483_RTS "UART_RTS"
#define	RN2483_CTS "UART_CTS"
#define	RN2483_TEST0 "TEST0"
#define	RN2483_TEST1 "TEST1"

class rn2483 {
public:
	rn2483(HardwareSerial &serial, int resetPin);
	bool initHardware();
	bool prepareABP(const String &addr, const String &appSKey,
			const String &nwkSKey);
	bool prepareOTAA(const String &devEUI, const String &appEUI,
			const String &appKey);
	bool join();
	bool isJoined();
	bool sleep(long ms);
	bool tx(String hexData, bool confirmed);
	bool poll();

	bool setNvm(int addr, char value);
	char getNvm(int addr);

	bool setPinmode(String pin, String mode);
	String getPinmode(String pin);

	bool setPin(String pin, bool state);
	bool getPin(String pin);

	int getAnalog(String pin);
	int getVDD();

	bool wakeUp();
	bool initFrequencyPlan();

	void printSettings(Stream &stream);

private:
	HardwareSerial &_rn2483Serial;
	int _resetPin;

	bool _sleeping = false;

	String _devEUI = "";
	String _appEUI = "";
	String _appKey = "";

	String _nwksKey = "";
	String _appsKEy = "";
	String _devKey = "";
	String _devAddr = "";

	FREQ_PLAN _frequencyPlan = DEFAULT_EU;

	bool _joined = false;
	bool _otaa = true;

	bool sendMacSet(const String &param, const String &value);
	bool sendMacSetCh(const String &param, unsigned int channel,
			const String &value);
	bool sendMacSetCh(const String &param, unsigned int channel,
			uint32_t value);
	bool sendMacSave();

	String sendMacGet(const String &param);
	String sendMacGetCh(const String &param, unsigned int channel);

	bool sendSetChannelDutyCycle(unsigned int channel, unsigned int dutyCycle);
	bool sendSetChannelFrequency(unsigned int channel, uint32_t frequency);
	bool sendSetChannelEnabled(unsigned int channel, bool enabled);
	bool sendSet2ndRecvWindow(unsigned int dataRate, uint32_t frequency);
	bool sendSetChannelDataRateRange(unsigned int channel,
			unsigned int minRange, unsigned int maxRange);

	String sendRaw(const String &cmd);
	bool sendRawExpectOk(const String &cmd);
	void clearSerial();
	void checkSleepBeforeCommand();
	char hex2byte(char c);
};


#endif /* RN2483_H_ */
