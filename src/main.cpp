#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

String message_buffer;
unsigned long bt_timer = 0;
unsigned long bt_wait_time = 20;
int red = 0;
int green = 0;
int blue = 0;

int color_cycle = 0;

unsigned long blink_timer = 0;
unsigned long blink_wait_time = 500;

bool blink_state = false;

enum Mode {
	STATIC,
	CYCLE, 
	BLINK
};

Mode mode = Mode::STATIC;

void setup() {
    SerialBT.begin("ESP32_Fernbedienung");

    u8g2.begin();      
    u8g2.setFont(u8g2_font_ncenB08_tr);

	message_buffer = "";

	bt_timer = millis();
	blink_timer = millis();

	// use 32, 33, 25 as pwm output
	ledcAttachPin(32, 0);
	ledcAttachPin(33, 1);
	ledcAttachPin(26, 2);

	// set pwm frequency to 5000 Hz
	ledcSetup(0, 5000, 8);
	ledcSetup(1, 5000, 8);
	ledcSetup(2, 5000, 8);

	// set pwm duty to 0
	ledcWrite(0, 0);
	ledcWrite(1, 0);
	ledcWrite(2, 0);
}
	

void loop() {
	u8g2.clearBuffer();
	if (SerialBT.connected()) {
		while (SerialBT.available()) {
			message_buffer += (char)SerialBT.read();
			bt_timer = millis();
		}
		if (millis() - bt_timer > bt_wait_time && !SerialBT.available() && !message_buffer.isEmpty()) {
			int first_slash = message_buffer.indexOf("/");
			int second_slash = message_buffer.indexOf("/", first_slash + 1);

			if (first_slash != -1 && second_slash != -1) {
				mode = Mode::STATIC;
				red = message_buffer.substring(0, first_slash).toInt();
				green = message_buffer.substring(first_slash + 1, second_slash).toInt();
				blue = message_buffer.substring(second_slash + 1).toInt();

				ledcWrite(2, red);
				ledcWrite(1, green);
				ledcWrite(0, blue);

				SerialBT.println("Farbe auf rgb(" + String(red) + ", " + String(green) + ", " + String(blue) + ") gesetzt!");
			} else {
				message_buffer.trim();
				if (message_buffer == "Cycle") {
					SerialBT.println("Cycle Modus aktiviert!");
					mode = Mode::CYCLE;
				} else if (message_buffer == "Blink") {
					SerialBT.println("Blink Modus aktiviert!");
					mode = Mode::BLINK;
				} else if (message_buffer == "Info") {
					SerialBT.println("Aktuelle Farbe: rgb(" + String(red) + ", " + String(green) + ", " + String(blue) + ")");
					switch (mode) {
						case Mode::CYCLE:
							SerialBT.println("Aktueller Modus: Cycle");
							break;
						case Mode::STATIC:
							SerialBT.println("Aktueller Modus: Static");
							break;
						case Mode::BLINK:
							SerialBT.println("Aktueller Modus: Blink");
							break;
					}
				} else {
					SerialBT.println("Unbekannter Befehl: " + message_buffer);
				}
			}
			message_buffer.clear();
		}
		u8g2.drawStr(3, 10, "Verbunden!");
	} else {
		u8g2.drawStr(3, 10, "Keine Verbindung!");
	}
	switch (mode) {
		case Mode::CYCLE:
			color_cycle += 1;
			if (color_cycle > 200) {
				color_cycle = 0;
			}
			red = (int)(sin(color_cycle / 200.0 * 2 * PI) * 127 + 128) / 3;
			green = (int)(sin(color_cycle / 200.0 * 2 * PI + 2 * PI / 3) * 127 + 128) / 3;
			blue = (int)(sin(color_cycle / 200.0 * 2 * PI + 4 * PI / 3) * 127 + 128) / 3;

			ledcWrite(2, red);
			ledcWrite(1, green);
			ledcWrite(0, blue);
			u8g2.drawStr(3, 60, "Mode: Cycle");
			break;
		case Mode::STATIC:
			u8g2.drawStr(3, 60, "Mode: Static");
			break;
		case Mode::BLINK:
			if (millis() - blink_timer > blink_wait_time) {
				blink_timer = millis();
				if (blink_state) {
					ledcWrite(2, red);
					ledcWrite(1, green);
					ledcWrite(0, blue);
				} else {
					ledcWrite(2, 0);
					ledcWrite(1, 0);
					ledcWrite(0, 0);
				}
				blink_state = !blink_state;
			}
			u8g2.drawStr(3, 60, "Mode: Blink");
			break;

	} 
	String line1 = "R: " + String(red);
	String line2 = "G: " + String(green);
	String line3 = "B: " + String(blue);
	u8g2.drawStr(3, 25, line1.c_str());
	u8g2.drawStr(3, 35, line2.c_str());
	u8g2.drawStr(3, 45, line3.c_str());
	u8g2.sendBuffer();
			
}	