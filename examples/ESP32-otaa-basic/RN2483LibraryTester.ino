//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "rn2483.h"

rn2483 myLora(Serial2, 5);

void setup() {
	Serial.begin(57600);
	delay(1000);
	while (Serial.available()) {
		Serial.read();
	}

	Serial.println("Init rn2483");
	myLora.initHardware();

	myLora.prepareOTAA("0004A30B00216B44", "70B3D57ED00001A6",
			"A23C96EE13804963F8C2BD6285448198");

	myLora.join();

	// myLora.printSettings(Serial);

	myLora.sleep(1000 * 60 * 10); // 10min

	Serial.println("VDD" + myLora.getVDD());


	bool v = myLora.tx("01", false);
	Serial.println("tx " + String(v ? "ok" : "failed"));
}

void loop() {
	if (Serial.available()) {
		Serial2.write(Serial.read());
	}
	if (Serial2.available()) {
		Serial.write(Serial2.read());
	}
}
