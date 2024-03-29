#include "Arduino.h"

#define DEVICES_NUM 8
#define DEVICE_PARAMS_NUM 4
#define COMM_TIME 300 //20	//s

// 0 - 4: INPUTS
// 4 - 8: OUTPUTS
#define MODE_2_OUT_IN		1
#define MODE_2_US			2
#define MODE_2_OUT_DHT22	3
#define MODE_2_OUT_COUNT	4
#define MODE_2_OUT_ONEWIRE	5
int mode;
int newMode;

#define CONFIG_WIFI_PIN 0
#define INPUT0_PIN 3	//RX	//5	//1	//TX
#define OUTPUT0_PIN 2
unsigned long outOnSecCounter[4];

//#define INPUT1_PIN 4	//2
//#define INPUT2_PIN 14	//3	//RX
//#define INPUT3_PIN 12	//3	//RX
//#define LED0_PIN 2		//16 //0 //D0 //14 //D5
//#define LED1_PIN 0 //D1 //12 //D6
//#define LED_BUILTIN 2

#include "libraries/DoubleResetDetector.h"
#define DRD_TIMEOUT 10
#define DRD_ADDRESS 0
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

//TODO: set time zone and year, month, day
//#include "Arduino.h"
//#include "libraries/WiFiManager/src/WiFiManager.h"
#define NTP
#ifdef NTP
#include "libraries/NTPClient.h"
#include "libraries/Timezone.h"
#endif
//#define ESP8266
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include "ESP8266HTTPClient.h"
#include <ESP8266HTTPUpdateServer.h>
//#include <WiFiManager.h>
#include "libraries/WiFiManager.h"

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

//const char* host = "esp8266-webupdate";
#else
#include <Update.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "libraries/OLED/SSD1306.h"
#include "libraries/OLED/OLEDDisplayUi.h"
//#include "images.h"
WebServer httpServer(80);
//const char* host = "esp32-webupdate";
#endif

#ifndef ESP8266
#define DRAWMESSAGE(display, message) (drawMessage(&display, message))
#else
#define display 0
#define DRAWMESSAGE(display, message) (drawMessage(message))
#endif

#include <WiFiClient.h>
#include <DNSServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

WiFiClient espClient;
IPAddress deviceIP;
bool isAP;
volatile int deviceCommIndex;
bool isCheckIn = false;
bool isErrorConn = false;
int reconnectTimeout = 0;

#define COUNTERS_NUM  1
#define COUNTERBUFFER_SIZE 64
unsigned long counter[COUNTERS_NUM];
unsigned long counters[COUNTERBUFFER_SIZE][COUNTERS_NUM];
uint8_t countersIndex;

//#define SD_CS_PIN 22
#ifdef SD_CS_PIN
#include <FS.h>
#include "libraries/SD/src/SD.h"
#include <SPI.h>
#endif

//WiFiManager wifiManager;
/*
 ESP8266
 static const uint8_t D0   = 16;
 static const uint8_t D1   = 5;
 static const uint8_t D2   = 4;
 static const uint8_t D3   = 0;
 static const uint8_t D4   = 2;
 static const uint8_t D5   = 14;
 static const uint8_t D6   = 12;
 static const uint8_t D7   = 13;
 static const uint8_t D8   = 15;
 static const uint8_t D9   = 3;
 static const uint8_t D10  = 1;
 */

#define ONEWIREBUS_PIN 3 //D8 //13 //D7
#ifdef ONEWIREBUS_PIN
//#include "libraries/OneWire.h"
#include "libraries/DallasTemperature.h"
OneWire oneWire(ONEWIREBUS_PIN);
DallasTemperature oneWireSensors(&oneWire);
#endif

//#define CONFIG_WIFI_PIN 27 //17 //D6 //5 //D1
//#define INPUT0_PIN 14 //D7 //4 //D2
//#define INPUT1_PIN 2 //35 //D7 //4 //D2
//#define INPUT2_PIN 15 //34 //D7 //4 //D2
//#define INPUT3_PIN 26 //32
//#define INPUT5_PIN 25 //33
//#define INPUT6_PIN 33 //25
//#define INPUT7_PIN 32 //26

#define DHT_PIN 3	//13	//12 //D2 //9//6
#ifdef DHT_PIN
#define DHT11 11	// DHT 11
#define DHT22 22	// DHT 22 (AM2302)
#define DHT21 21	// DHT 21 (AM2301)
#include "libraries/DHT.h"
DHT dht(DHT_PIN, DHT22);
//DHT* pDHT;
#endif

#define MQTT
#ifdef MQTT
#include <PubSubClient.h>
PubSubClient mqttClient(espClient);
//std::atomic_flag mqttLock = ATOMIC_FLAG_INIT;
String mqttRootTopicVal;
String mqttRootTopicState;
int mqttState;
#endif

#ifdef NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = { "CEST", Last, Sun, Mar, 2, 120 }; //Central European Summer Time
TimeChangeRule CET = { "CET ", Last, Sun, Oct, 3, 60 }; //Central European Standard Time
Timezone CE(CEST, CET);
#endif

#define OUTPUT_BIT 0
#define MANUAL_BIT 1
#define CMD_BIT 2
#define UNACK_BIT 3
#define RUNONCE_BIT 4
#define PREVOUTPUT_BIT 5

#ifdef MQTT
//const char* mqtt_server = "broker.hivemq.com";//"iot.eclipse.org";
//const char* mqtt_server = "iot.eclipse.org";

#define MQTT_CLIENTID   "ESPNODE-"
//#define MQTT_ROOT_TOPIC	"ESPNODE/"
//#define A_TOPIC 	"/A"
//#define B_TOPIC 	"/B"
//#define C_TOPIC 	"/C"
//#define D_TOPIC 	"/D"
//#define E_TOPIC 	"/E"
//#define F_TOPIC 	"/F"

//#define A_VAL_TOPIC 	"/A/val"
//#define A_CMP_TOPIC     "/A/cmd" /* 1=on, 0=off */
//#define B_VAL_TOPIC 	"/B/val"
//#define B_CMP_TOPIC     "/B/cmd" /* 1=on, 0=off */
//#define C_VAL_TOPIC 	"/C/val"
//#define C_CMP_TOPIC     "/C/cmd" /* 1=on, 0=off */
//#define D_VAL_TOPIC 	"/D/val"
//#define D_CMP_TOPIC     "/D/cmd" /* 1=on, 0=off */
char msg[20];
#endif

#ifdef SD_CS_PIN
bool loadFromSdCard(String path) {
	String dataType = "text/plain";
	/*
	if(path.endsWith("/")) path += "index.htm";
	if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
	else if(path.endsWith(".htm")) dataType = "text/html";
	else if(path.endsWith(".css")) dataType = "text/css";
	else if(path.endsWith(".js")) dataType = "application/javascript";
	else if(path.endsWith(".png")) dataType = "image/png";
	else if(path.endsWith(".gif")) dataType = "image/gif";
	else if(path.endsWith(".jpg")) dataType = "image/jpeg";
	else if(path.endsWith(".ico")) dataType = "image/x-icon";
	else if(path.endsWith(".xml")) dataType = "text/xml";
	else if(path.endsWith(".pdf")) dataType = "application/pdf";
	else if(path.endsWith(".zip")) dataType = "application/zip";
	*/
	File dataFile = SD.open(path.c_str());
	//if(dataFile.isDirectory()){
	//	path += "/index.htm";
	//  dataType = "text/html";
	//  dataFile = SD.open(path.c_str());
	//}
	if (!dataFile)
		return false;
	//if (httpServer.hasArg("download")) dataType = "application/octet-stream";
	if (httpServer.streamFile(dataFile, dataType) != dataFile.size()) {
		Serial.println("Sent less data than expected!");
	}
	dataFile.close();
	return true;
}
#endif

#ifndef ESP8266
#define SAMPLES 16
float analogRead(int pin, int samples) {
	float r = 0;
	for (int i = 0; i < samples; i++)
		r += analogRead(pin);
	r /= samples;
	return r;
}
#endif

struct Device {
	//int par0;
	//int par1;
	//int par2;
	//int par3;
	int par[DEVICE_PARAMS_NUM];
	int flags;
	char name[16];
	float val;
} devices[DEVICES_NUM];

#ifdef SD_CS_PIN
bool isSD, errorSD;
#endif

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#ifndef ESP8266
// OLED pins to ESP32 GPIOs via this connection:
#define OLED_ADDRESS 0x3c
#define OLED_SDA 5
#define OLED_SCL 4
//#define OLED_RST 16 // GPIO16

SSD1306 display(OLED_ADDRESS, OLED_SDA, OLED_SCL);

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
	display->setTextAlignment(TEXT_ALIGN_RIGHT);
	display->setFont(ArialMT_Plain_16);
	//display->drawString(128, 0, String(millis()));
	//display->drawString(128, 0, timeClient.getFormattedTime());

	time_t t = CE.toLocal(timeClient.getEpochTime());
	byte h = (t / 3600) % 24;
	byte m = (t / 60) % 60;
	byte s = t % 60;

	char buff[10];
	sprintf(buff, "%02d:%02d:%02d", h, m, s);
	display->drawString(128, 0, buff);

	display->setTextAlignment(TEXT_ALIGN_LEFT);
	if(isAP)
		display->drawString(0, 0, "AP");
	else {
		if(isErrorConn)
			display->drawString(0, 0, "OFFLIN");
		else
			display->drawString(0, 0, "ONLINE");
	}
}

#define X2 128
void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	//display->drawXbm(x + 34, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	if(!isAP) {
		display->drawString(0 + x, 16 + y, WiFi.localIP().toString());
	}
	else {
		display->drawString(0 + x, 16 + y, deviceIP.toString());
	}
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0 + x, 32 + y, "ESPNODE");
}

void drawFrameA1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0 + x, 16 + y, "LEVEL ALARMS");
	if(bitRead(devices[3].flags, OUTPUT_BIT))
		display->drawString(0 + x, 32 + y, "MIN");
	else
		display->drawString(0 + x, 32 + y, "    ");
	if(bitRead(devices[4].flags, OUTPUT_BIT))
		display->drawString(64 + x, 32 + y, "MAX");
	else
		display->drawString(64 + x, 32 + y, "   ");
}

void drawFrameA2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	if(!bitRead(devices[DEV_ALARM_MAX].flags, OUTPUT_BIT)) {
		drawNextFrame(display);
		return;
	}
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0 + x, 16 + y, "ALARM");
    display->drawString(0 + x, 32 + y, "LEVEL MAX");
}

void drawFrameA3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	if(!bitRead(devices[DEV_ALARM_MIN].flags, OUTPUT_BIT)) {
		drawNextFrame(display);
		return;
	}
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0 + x, 16 + y, "ALARM");
    display->drawString(0 + x, 32 + y, "LEVEL MIN");
}

void drawFrameA4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
#ifdef SD_CS_PIN
	if(!errorSD) {
		drawNextFrame(display);
		return;
	}
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0 + x, 16 + y, "ALARM");
    display->drawString(0 + x, 32 + y, "SD CARD ERR");
#else
    drawNextFrame(display);
#endif
}

void drawFrameA5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	if(!isErrorConn) {
		drawNextFrame(display);
		return;
	}
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0 + x, 16 + y, "ALARM");
    display->drawString(0 + x, 32 + y, "INTERNET ERR");
}

String deviceToString(struct Device device){
	//return String(bitRead(device.flags, MANUAL_BIT) ? "MAN " : "AUTO ")  + String(bitRead(device.flags, OUTPUT_BIT) ? "ON" : "OFF");
	return String(bitRead(device.flags, OUTPUT_BIT) ? "OPEN" : "CLOSE");
}

void drawFrameD1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0 + x, 16 + y,"A: " + String(devices[0].name));
    display->setFont(ArialMT_Plain_16);
    display->drawString(0 + x, 32 + y, deviceToString(devices[0]));
}

void drawFrameD2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0 + x, 16 + y, "B: " + String(devices[1].name));
    display->setFont(ArialMT_Plain_16);
    display->drawString(0 + x, 32 + y, deviceToString(devices[1]));
}

void drawFrameD3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0 + x, 16 + y, "C: " + String(devices[2].name));
    display->setFont(ArialMT_Plain_16);
    display->drawString(0 + x, 32 + y, deviceToString(devices[2]));
}

void drawFrameD4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0 + x, 16 + y, "D: " + String(devices[3].name));
    display->setFont(ArialMT_Plain_16);
    display->drawString(0 + x, 32 + y, deviceToString(devices[3]));
}

void drawFrameM1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0 + x, 16 + y, "LEVEL");
	display->setFont(ArialMT_Plain_16);
	//levelRaw = analogRead(A0, SAMPLES);
	//level = levelRaw * levelK + levelD;
	display->drawString(0 + x, 32 + y, String(level));
}

FrameCallback frames[] = { drawFrame1, drawFrameA2, drawFrameA3, drawFrameA4, drawFrameA5, drawFrameD1, drawFrameD2, drawFrameD3, drawFrameD4, drawFrameM1};
int frameCount = 10;
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;
int frameNo = 0;
int lastFrameNo = 0;
String lastMessage;

void drawDisplay(OLEDDisplay *display) {
	drawDisplay(display, lastFrameNo);
}

void drawDisplay(OLEDDisplay *display, int frame) {
	lastFrameNo = frame;
	display->clear();
	(frames[frame])(display, 0, 0, 0);
	for (uint8_t i=0; i<overlaysCount; i++){
	    (overlays[i])(display, 0 );
	 }
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	for(int i = 0; i < 4; i++) {
		if(bitRead(devices[i].flags, OUTPUT_BIT)) {
			char ch = 65 + i;
			display->drawString(12 * i, 48, String(ch));
		}
	}
	if(isErrorConn) {
		display->setTextAlignment(TEXT_ALIGN_RIGHT);
		display->drawString(128, 48, "ALM");
		display->setTextAlignment(TEXT_ALIGN_LEFT);
	}

#ifdef SD_CS_PIN
	if(errorSD) {
		display->setTextAlignment(TEXT_ALIGN_RIGHT);
		display->drawString(128, 48, "ALM");
		display->setTextAlignment(TEXT_ALIGN_LEFT);
	}
#endif

	/*
	for(int i = 0; i < 4; i++) {
		display->drawString(25 * i, 0, String(i));
		display->drawXbm(25 * i, 0, 25, 25, dropSymbol);
		if(!bitRead(devices[i].flags, OUTPUT_BIT))
			display->drawXbm(25 * i, 0, 25, 25, crossSymbol);
	}
	*/
	display->display();
}
void drawMessage(OLEDDisplay *display, String msg) {
	lastMessage = msg;
	Serial.println(msg);
}
#else

void drawMessage(String msg) {
	Serial.println(msg);
}
#endif

#ifdef MQTT
void receivedCallback(char* topic, byte* payload, unsigned int length) {
	Serial.print("MQTT TOPIC: ");
	Serial.print(topic);
	Serial.print(" PAYLOAD: ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println();

	if(strstr(topic, "/parsave")) {
		saveInstruments();
		return;
	}
	if(strstr(topic, "/parload")) {
		String mqttTopicVal = String(mqttRootTopicVal + "/mode");
		snprintf(msg, 20, "%i", mode);
		mqttClient.publish(mqttTopicVal.c_str(), msg);
		Serial.print(mqttTopicVal);
		Serial.print(": ");
		Serial.println(msg);

		for(int i = 0; i < DEVICES_NUM; i++) {
			for(int p = 0;  p < DEVICE_PARAMS_NUM; p++) {
				String mqttTopic = String('/' + String(i) + "/par" + String(p));
				String mqttTopicVal = String(mqttRootTopicVal + mqttTopic);
				snprintf(msg, 20, "%i", devices[i].par[p]);
				mqttClient.publish(mqttTopicVal.c_str(), msg);
				Serial.print(mqttTopicVal);
				Serial.print(": ");
				Serial.println(msg);
			}
		}
		return;
	}

	if(strstr((topic + strlen(topic) - 5), "/par")) {
		int p = topic[strlen(topic) - 1] - 48;
		int i = topic[strlen(topic) - 6] - 48;
		//Serial.println(i);
		//Serial.println(p);

		if(i > -1 && i < DEVICES_NUM && p > -1 && p < DEVICE_PARAMS_NUM) {
			strncpy(msg, (const char*)payload, length);
			msg[length] = 0;
			devices[i].par[p] = atoi(msg);

			String mqttTopic = String('/' + String(i) + "/par" + String(p));
			String mqttTopicVal = String(mqttRootTopicVal + mqttTopic);
			snprintf(msg, 20, "%i", devices[i].par[p]);
			mqttClient.publish(mqttTopicVal.c_str(), msg);
			Serial.print(mqttTopicVal);
			Serial.print(": ");
			Serial.println(msg);
		}
		return;
	}

	int i = topic[strlen(topic) - 1] - 48;
	/*
	int i = -1;
	if(strstr(topic, A_TOPIC))
		i = 0;
	if(strstr(topic, B_TOPIC))
		i = 1;
	if(strstr(topic, C_TOPIC))
		i = 2;
	if(strstr(topic, D_TOPIC))
		i = 3;
	if(strstr(topic, E_TOPIC))
		i = 4;
	if(strstr(topic, F_TOPIC))
		i = 5;
	*/
	if(i > -1 && i < DEVICES_NUM) {
		if((char)payload[0]=='0') {
			bitClear(devices[i].flags, OUTPUT_BIT);
			bitSet(devices[i].flags, MANUAL_BIT);
		}
		else if((char)payload[0]=='1') {
			bitSet(devices[i].flags, OUTPUT_BIT);
			bitSet(devices[i].flags, MANUAL_BIT);
		}
		//else if((char)payload[0])=='A') {
		//	bitClear(devices[0].flags, RUNONCE_BIT);
		//  bitClear(devices[0].flags, MANUAL_BIT);
		//}
	}
}

void mqttConnect() {
	int i = 0;
	while (!mqttClient.connected()) {
		yield();
		//Serial.print("MQTT connecting ...");
		if(mqttClient.connect(mqttClientId.c_str(), mqttUser, mqttPassword)) {;
			mqttClient.subscribe(String(mqttRootTopic + String(mqttID) + "/cmd/#").c_str());
			mqttState = mqttClient.state();

			Serial.println(String(mqttRootTopic + String(mqttID) + "/cmd/#"));
			Serial.print("Status code = ");
			Serial.println(mqttState);
			break;

		}
		else {
			mqttState = mqttClient.state();
			Serial.print("Failed, status code = ");
			Serial.println(mqttState);
			delay(1000);
		}
		if(i++ >= 2)
			break;
	}
}
#endif

const char* host = "ESPNODE-ESP";
const char* update_path = "/firmware";

char* htmlHeader =
		"<html><head><title>ESPNODE</title><meta name=\"viewport\" content=\"width=device-width\"><style type=\"text/css\">body{font-family:monospace;} input{padding:5px;font-size:1em;font-family:monospace;} button{height:100px;width:100px;font-family:monospace;border-radius:5px;}</style></head><body><h1><a href=/>ESPNODE</a></h1>";
char* htmlFooter =
		"<hr><a href=/save>SAVE SETTINGS!</a><hr><a href=/settings>SYSTEM SETTINGS</a><hr>(c) GROWMAT EASY</body></html>";
//const char HTTP_STYLE[] PROGMEM  = "<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;} .l{background: url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==\") no-repeat left center;background-size: 1em;}</style>";
//const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='UPDATE'></form>";

const char* www_username = "ESPNODE";
char www_password[20];

#define THINGSSPEAK
#ifdef THINGSSPEAK
char serverName[20];
char writeApiKey[20];
unsigned int talkbackID;
char talkbackApiKey[20];
#endif

#ifdef MQTT
char mqttServer[20];
char mqttUser[20];
char mqttPassword[20];
char mqttRootTopic[20];
unsigned int mqttID;
String mqttClientId;
#endif

HTTPClient http;
int httpCode;
unsigned int httpErrorCounter;

/*
 ESP8266
 static const uint8_t D0   = 16;
 static const uint8_t D1   = 5;
 static const uint8_t D2   = 4;
 static const uint8_t D3   = 0;
 static const uint8_t D4   = 2;
 static const uint8_t D5   = 14;
 static const uint8_t D6   = 12;
 static const uint8_t D7   = 13;
 static const uint8_t D8   = 15;
 static const uint8_t D9   = 3;
 static const uint8_t D10  = 1;
 */

//#define LCD
#ifdef LCD
#include <SPI.h>
#include "libraries/Adafruit_GFX.h"
#include "libraries/Adafruit_PCD8544.h"
// Pins
const int8_t RST_PIN = 4; //D2;
const int8_t CE_PIN = 5; //D1;
const int8_t DC_PIN = 12; //D6;
//const int8_t DIN_PIN = D7;  // Uncomment for Software SPI
//const int8_t CLK_PIN = D5;  // Uncomment for Software SPI
const int8_t BL_PIN = 16; //D0;
Adafruit_PCD8544 display = Adafruit_PCD8544(DC_PIN, CE_PIN, RST_PIN);
#endif

#ifdef RXTX_PIN
unsigned int r1Off = 2664494;
unsigned int r1On = 2664495;
unsigned int r2Off = 2664492;
unsigned int r2On = 2664493;
unsigned int r3Off = 2664490;
unsigned int r3On = 2664491;
unsigned int r4Off = 2664486;
unsigned int r4On = 2664487;
unsigned int rAllOff = 2664481;
unsigned int rAllOn = 2664482;

#include "libraries/RCSwitch.h"
RCSwitch rcSwitch = RCSwitch();
//#define RFRX_PIN D3
#endif

//#include "libraries/interval.h"
//Interval minInterval, secInterval;
unsigned long lastMillis;

unsigned long secCounter = 0;

#define EEPROM_FILANEME_ADDR 124
#define EEPROM_OFFSET 177 //64 //8

String getDeviceForm(int i, struct Device devices[]) {
	Device d = devices[i];
	String s = "<form action=/dev><input type=hidden name=id value=";
	s += i;
	s += "><h2>ID ";
	s += char(i + 48);
	//s += String(d.par3);
	s += ": ";
	s += String(d.name);

	//	if(bitRead(devices[i].flags, MANUAL_BIT))
	//		s += " MANUAL";
	//	else
	//		s += " AUTO";
	s += "<br>";

	if (i >= 4) {
		s += "STATE: ";
		s += String(bitRead(devices[i].flags, OUTPUT_BIT));
		s += "<br>";
		s += "VALUE: ";
		s += String(devices[i].val);
		s += "</h2>";
	}
	else {
		if(mode == MODE_2_OUT_COUNT) {
			//if (i == 0) {
			s += "ALARM: ";
			s += String(bitRead(devices[i].flags, OUTPUT_BIT));
			s += "<br>";
			s += "COUNTER [-]: ";
			s += String(devices[i].val);
			s += "</h2>";
			//}
		}

		if(mode == MODE_2_US) {
			//if (i == 0) {
			s += "ALARM: ";
			s += String(bitRead(devices[i].flags, OUTPUT_BIT));
			s += "<br>";
			s += "DISTANCE [mm]: ";
			s += String(devices[i].val);
			s += "</h2>";
			//}
		}

		if(mode == MODE_2_OUT_ONEWIRE) {
			s += "ALARM: ";
			s += String(bitRead(devices[i].flags, OUTPUT_BIT));
			s += "<br>";
			s += "TEMPERATURE [C]: ";
			s += String(devices[i].val);
			s += "</h2>";
		}

		if(mode == MODE_2_OUT_DHT22) {
			if (i == 0) {
				s += "ALARM: ";
				s += String(bitRead(devices[i].flags, OUTPUT_BIT));
				s += "<br>";
				s += "TEMPERATURE [C]: ";
				s += String(devices[i].val);
				s += "</h2>";
			}
			if (i == 1) {
				s += "ALARM: ";
				s += String(bitRead(devices[i].flags, OUTPUT_BIT));
				s += "<br>";
				s += "HUMIDITY [%]: ";
				s += String(devices[i].val);
				s += "</h2>";
			}
			if(i > 1) {
				s += "STA: ";
				s += String(bitRead(devices[i].flags, OUTPUT_BIT));
				s += "</br>";
				s += "VAL: ";
				s += String(devices[i].val);
				s += "</h2>";
			}
		}
	}
	//	if (bitRead(devices[i].flags, OUTPUT_BIT))
	//		s += " ON";
	//	else
	//		s += " OFF";
	//}
	//s += "</h2>";
	if (i >= 4) {
		//s += "<button type=submit name=cmd value=off>OFF</button>&nbsp;&nbsp;&nbsp;<button type=submit name=cmd value=on>ON</button>&nbsp;&nbsp;&nbsp;<button type=submit name=cmd value=auto>AUTO</button>";
		s += "<button type=submit name=cmd value=off>OFF</button>&nbsp;&nbsp;&nbsp;<button type=submit name=cmd value=on>ON</button>";
	}
	//s += "<h2>SETTINGS</h2>";
	s += "<hr>NAME<br>";
	s += "<input name=name value=\"";
	s += d.name;
	s += "\">";

	/*
	if (i < DEVICES_NUM) {
		s += "<hr>par0<br><input name=par0 value=";
		s += d.par0;
		s += "><hr>par1<br><input name=par1 value=";
		s += d.par1;
		s += "><hr>par2<br><input name=par2 value=";
		s += d.par2;
		s += "><hr>par3<br><input name=par3 value=";
		s += d.par3;
		s += ">";
	}
	*/
	if (i >= 4) {
		s += "<hr>TIME ON [h]<br><input name=par0 value=";
		s += d.par[0];
		s += "><br><br>TIME ON [m]<br><input name=par1 value=";
		s += d.par[1];
		s += "><br><br>TIME OFF [m]<br><input name=par2 value=";
		s += d.par[2];
		s += "><br><br>TIME OFF [s]<br><input name=par3 value=";
		s += d.par[3];
		s += ">";
	}

	if (i < 4) {
		if(mode == MODE_2_OUT_COUNT) {
			s += "<hr>PULSES<br><input name=par0 value=";
			s += d.par[0];
			s += "><br><br>TIME [s]<br><input name=par1 value=";
			s += d.par[1];
			s += ">";
		}
		else {
			s += "<hr>HIGH ALARM<br><input name=par0 value=";
			s += d.par[0];
			s += "><br><br>LOW ALARM<br><input name=par1 value=";
			s += d.par[1];
			s += ">";
		}
		s += "<br><br>PAR2<br><input name=par2 value=";
		s += d.par[2];
		s += "><br><br>PAR3<br><input name=par3 value=";
		s += d.par[3];
		s += ">";
	}

	s += "<hr><button type=submit name=cmd value=set>SET</button>";
	s += "</form>";
	return s;
}

void handleRoot() {
	if (!httpServer.authenticate(www_username, www_password))
		return httpServer.requestAuthentication();

	Serial.println("Enter handleRoot");
	String message;
	httpServer.send(200, "text/plain", message);
}

void saveApi() {
	EEPROM.put(4, newMode);

	int offset = 8;
	EEPROM.put(offset, www_password);
	offset += sizeof(www_password);

#ifdef THINGSSPEAK
	EEPROM.put(offset, serverName);
	offset += sizeof(serverName);
	EEPROM.put(offset, writeApiKey);
	offset += sizeof(writeApiKey);
	EEPROM.put(offset, talkbackApiKey);
	offset += sizeof(talkbackApiKey);
	EEPROM.put(offset, talkbackID);
	offset += sizeof(talkbackID);
#endif

#ifdef MQTT
	EEPROM.put(offset, mqttServer);
	offset += sizeof(mqttServer);
	EEPROM.put(offset, mqttUser);
	offset += sizeof(mqttUser);
	EEPROM.put(offset, mqttPassword);
	offset += sizeof(mqttPassword);
	EEPROM.put(offset, mqttID);
	offset += sizeof(mqttID);
	EEPROM.put(offset, mqttRootTopic);
	offset += sizeof(mqttRootTopic);
#endif

	EEPROM.put(0, 0);
	EEPROM.commit();

#ifdef MQTT
	mqttRootTopicVal = String(mqttRootTopic + String(mqttID) + "/val");
	mqttRootTopicState = String(mqttRootTopic + String(mqttID) + "/sta");
	mqttClient.setServer(mqttServer, 1883);
#endif

}

void saveInstruments() {
	for (int i = 0; i < DEVICES_NUM; i++) {
		EEPROM.put(EEPROM_OFFSET + sizeof(Device) * i, devices[i]);
	}
	EEPROM.put(0, 0);
	EEPROM.commit();
}

void startWiFiAP() {
	isAP = true;
	WiFi.softAP("ESPNODE", "ESPNODE");
	Serial.println("Starting AP ...");
	deviceIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(deviceIP);
}

void handleInterrupt0() {
	counter[0]++;
}

void handleInterrupt1() {
	counter[1]++;
}

void handleInterrupt2() {
	counter[2]++;
}

void handleInterrupt3() {
	counter[3]++;
}

bool connectWiFi() {

	if(WiFi.status() == WL_CONNECTED) {
		digitalWrite(LED_BUILTIN, true);
		return true;
	}
	DRAWMESSAGE(display, "WIFI CONN... ");
	WiFiManager wifiManager;
	//if (wifiManager.autoConnect()) {
	if (wifiManager.connectWifi("", "") == WL_CONNECTED) {
		isAP = false;
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());
		DRAWMESSAGE(display, "WIFI CONN");

#ifdef LED0_PIN
		digitalWrite(LED0_PIN, true);
#endif

		return true;
	}

#ifdef LED0_PIN
	for(int i = 0; i < 16; i++) {
		digitalWrite(LED0_PIN, true);
		delay(250);
		digitalWrite(LED0_PIN, false);
		delay(250);
	}
#endif

	return false;
}

/////////////////////////////////
// setup
/////////////////////////////////
void setup() {

	//pinMode(LED_BUILTIN, OUTPUT);
	//digitalWrite(LED_BUILTIN, true);
	//pinMode(16, OUTPUT);
	//digitalWrite(16, true);

	//for(int i = 0; i < COUNTERS_NUM; i++) {
	//	for(int j = 0; j < COUNTERBUFFER_SIZE; j++)
	//		counters[j][i] = 0;
	//}

#ifdef INPUT0_PIN
	//GPIO 3 (RX) swap the pin to a GPIO
	pinMode(3, FUNCTION_3);
	pinMode(3, INPUT);

#elif INPUT2_PIN
	Serial.begin(9600, SERIAL_8N1, SERIAL_TX_ONLY);//, 1);
#else
	Serial.begin(9600);
#endif
	//Serial.begin(9600);
	Serial.begin(9600, SERIAL_8N1, SERIAL_TX_ONLY);

	Serial.print("\n\n");
	Serial.println("ESPNODE");

#ifndef ESP8266
	//analogReadResolution(9);
	analogReadResolution(12);
	display.init();
	display.setFont(ArialMT_Plain_24);
	display.setTextAlignment(TEXT_ALIGN_LEFT);
	display.drawString(0, 0, "ESPNODE");
	display.display();
#endif

#ifdef SD_CS_PIN
	isSD = true;
	if(!SD.begin(SD_CS_PIN)) {
		isSD = false;
		Serial.println("Card Mount Failed");
	}
	uint8_t cardType = SD.cardType();
	if(cardType == CARD_NONE){
		isSD = false;
		Serial.println("No SD card attached");
	}
	errorSD = !isSD;
#endif

#ifdef LCD
	display.begin();
	display.setContrast(60);
	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(BLACK);
	display.setCursor(0,0);
	display.println("ESPNODE");
	display.display();
#endif

#ifdef ONEWIREBUS_PIN;
	oneWireSensors.begin();
#endif

#ifdef RFRX_PIN
	rcSwitch.enableReceive(RFRX_PIN);
#endif

//#define RFTX_PIN 22 //D5
#ifdef RFTX_PIN
	rcSwitch.enableTransmit(RFTX_PIN);
#endif

	EEPROM.begin(512);
	if (!EEPROM.read(0)) {
		mode = EEPROM.read(4);
		newMode = mode;

		int offset = 8;
		EEPROM.get(offset, www_password);
		offset += sizeof(www_password);

#ifdef THINGSSPEAK
		EEPROM.get(offset, serverName);
		offset += sizeof(serverName);
		EEPROM.get(offset, writeApiKey);
		offset += sizeof(writeApiKey);
		EEPROM.get(offset, talkbackApiKey);
		offset += sizeof(talkbackApiKey);
		EEPROM.get(offset, talkbackID);
		offset += sizeof(talkbackID);
#endif

#ifdef MQTT
		EEPROM.get(offset, mqttServer);
		offset += sizeof(mqttServer);
		EEPROM.get(offset, mqttUser);
		offset += sizeof(mqttUser);
		EEPROM.get(offset, mqttPassword);
		offset += sizeof(mqttPassword);
		EEPROM.get(offset, mqttID);
		offset += sizeof(mqttID);
		mqttClient.setServer(mqttServer, 1883);
		EEPROM.get(offset, mqttRootTopic);
		offset += sizeof(mqttRootTopic);
#endif

		for (int i = 0; i < DEVICES_NUM; i++) {
			EEPROM.get(EEPROM_OFFSET + sizeof(Device) * i, devices[i]);
			bitClear(devices[i].flags, OUTPUT_BIT);
			devices[i].val = 0;
		}

	} else {
		strcpy(www_password, "ESPNODE");

		for (int i = 0; i < DEVICES_NUM; i++) {
			if(i < 4) {
				strcpy(devices[i].name, "IN ");
				devices[i].name[3] = i + 48;
				devices[i].name[4] = 0;
			}
			else {
				strcpy(devices[i].name, "OUT ");
				devices[i].name[4] = i + 48;
				devices[i].name[5] = 0;
			}
			devices[i].par[0] = 0;
			devices[i].par[1] = 0;
			devices[i].par[2] = 0;
			devices[i].par[3] = i;
			bitClear(devices[i].flags, OUTPUT_BIT);
		}

#ifdef THINGSSPEAK
		//strcpy(serverName, "api.thingspeak.com");
		//writeApiKey[0] = '/0';
		//talkbackID = 0;

		strcpy(serverName, "192.168.0.1");
		strcpy(writeApiKey, "update");
		talkbackID = 80;
		talkbackApiKey[0] = '/0';
#endif

#ifdef MQTT
		//strcpy(mqttUser, "ESPNODE") ;
		//strcpy(mqttPassword, "ESPNODE") ;

		strcpy(mqttServer, "broker.hivemq.com");
		mqttUser[0] = '/0';
		mqttPassword[0] = '/0';
		mqttID = 0;
		strcpy(mqttRootTopic, "ESPNODE/");
#endif

		saveApi();
		saveInstruments();
	}

#ifdef MQTT
	mqttRootTopicVal = String(mqttRootTopic + String(mqttID) + "/val");
	mqttRootTopicState = String(mqttRootTopic + String(mqttID) + "/sta");
#endif

#ifdef CONFIG_WIFIAP_PIN
	pinMode(CONFIG_WIFIAP_PIN, INPUT_PULLUP);
#endif

	Serial.print("MODE: ");
	Serial.println(mode);

	if(mode == MODE_2_US) {
#ifdef INPUT0_PIN
		pinMode(INPUT0_PIN, INPUT);
		pinMode(OUTPUT0_PIN, OUTPUT);
#endif
	}

	if(mode == MODE_2_OUT_DHT22 || mode == MODE_2_OUT_ONEWIRE) {
#ifdef DHT_PIN
		if(mode == MODE_2_OUT_DHT22) {
			//dht.begin();
			//DHT dht(DHT_PIN, DHT22);
			dht.begin();
			//pDHT = &dht;
		}
#endif
#ifdef ONEWIREBUS_PIN;
		if(mode == MODE_2_OUT_ONEWIRE) {
			oneWireSensors.begin();
		}
#endif
#ifdef OUTPUT0_PIN
		digitalWrite(OUTPUT0_PIN, false);
		pinMode(OUTPUT0_PIN, OUTPUT);
#endif
	}

	if(mode == MODE_2_OUT_IN) {
#ifdef INPUT0_PIN
		pinMode(INPUT0_PIN, INPUT);
#endif
#ifdef INPUT1_PIN
		pinMode(INPUT1_PIN, INPUT);
#endif
#ifdef INPUT2_PIN
		pinMode(INPUT2_PIN, INPUT);
#endif
#ifdef INPUT3_PIN
		pinMode(INPUT3_PIN, INPUT);
#endif
#ifdef OUTPUT0_PIN
		digitalWrite(OUTPUT0_PIN, false);
		pinMode(OUTPUT0_PIN, OUTPUT);
#endif
#ifdef OUTPUT1_PIN
		digitalWrite(OUTPUT1_PIN, false);
		pinMode(OUTPUT1_PIN, OUTPUT);
#endif
#ifdef OUTPUT2_PIN
		digitalWrite(OUTPUT2_PIN, false);
		pinMode(OUTPUT2_PIN, OUTPUT);
#endif
#ifdef OUTPUT3_PIN
		digitalWrite(OUTPUT3_PIN, false);
		pinMode(OUTPUT3_PIN, OUTPUT);
#endif
	}

	if(mode == MODE_2_OUT_COUNT) {
#ifdef INPUT0_PIN
		//pinMode(INPUT0_PIN, FUNCTION_3);
		pinMode(INPUT0_PIN, INPUT);
		attachInterrupt(digitalPinToInterrupt(INPUT0_PIN), handleInterrupt0, FALLING);
#endif
#ifdef OUTPUT0_PIN
		digitalWrite(OUTPUT0_PIN, false);
		pinMode(OUTPUT0_PIN, OUTPUT);
#endif
#ifdef INPUT1_PIN
		pinMode(INPUT1_PIN, INPUT_PULLUP);
		attachInterrupt(digitalPinToInterrupt(INPUT1_PIN), handleInterrupt1, FALLING);
#endif
#ifdef INPUT2_PIN
		pinMode(INPUT2_PIN, FUNCTION_3);
		pinMode(INPUT2_PIN, INPUT_PULLUP);
		attachInterrupt(digitalPinToInterrupt(INPUT2_PIN), handleInterrupt2, FALLING);
#endif
#ifdef INPUT3_PIN
		pinMode(INPUT3_PIN, FUNCTION_3);
		pinMode(INPUT3_PIN, INPUT_PULLUP);
		attachInterrupt(digitalPinToInterrupt(INPUT3_PIN), handleInterrupt3, FALLING);
#endif
	}

	/*
	//GPIO 1 (TX) swap the pin to a GPIO.
	pinMode(1, FUNCTION_3);
	//GPIO 3 (RX) swap the pin to a GPIO.
	pinMode(3, FUNCTION_3);

	//GPIO 1 (TX) swap the pin to a TX.
	pinMode(1, FUNCTION_0);
	//GPIO 3 (RX) swap the pin to a RX.
	pinMode(3, FUNCTION_0);
	*/

#ifdef CONFIG_WIFI_PIN
	pinMode(CONFIG_WIFI_PIN, INPUT_PULLUP);
#endif

#ifdef RFTX_PIN
	pinMode(RFTX_PIN, OUTPUT);
#endif

#ifdef OUTPUT1_PIN
	pinMode(OUTPUT0_PIN, OUTPUT);
	pinMode(OUTPUT1_PIN, OUTPUT);
	pinMode(OUTPUT2_PIN, OUTPUT);
	pinMode(OUTPUT3_PIN, OUTPUT);
	digitalWrite(OUTPUT0_PIN, HIGH);
	digitalWrite(OUTPUT1_PIN, HIGH);
	digitalWrite(OUTPUT2_PIN, HIGH);
	digitalWrite(OUTPUT3_PIN, HIGH);
#endif

#ifdef LED0_PIN
	pinMode(LED0_PIN, OUTPUT);
	digitalWrite(LED0_PIN, false);
#endif

#ifdef LED1_PIN
	pinMode(LED1_PIN, OUTPUT);
	digitalWrite(LED1_PIN, LOW);
#endif

#ifdef MQTT
	mqttClientId = String(MQTT_CLIENTID) + WiFi.macAddress();
	Serial.println(mqttClientId);
#endif


	//WiFiManager
	//Local intialization. Once its business is done, there is no need to keep it around
	//reset saved settings
	//wifiManager.resetSettings();
	//set custom ip for portal
	//wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
	//fetches ssid and pass from eeprom and tries to connect
	//if it does not connect it starts an access point with the specified name
	//here  "AutoConnectAP"
	//and goes into a blocking loop awaiting configuration

	//WiFiManager wifiManager;


#ifdef INPUT0_PIN
	if (digitalRead(INPUT0_PIN) == LOW) {
		strcpy(www_password, "ESPNODE");
	}
#endif

//#ifdef CONFIG_WIFI_PIN
	/*
	bool isConfigWifi = false;
	while(millis() < 10000) {
		yield();
		if(millis() < 5000)

			continue;
		if (digitalRead(CONFIG_WIFI_PIN) == LOW) {
			isConfigWifi = true;
			break;
		}
	}
	if (isConfigWifi) {
	//if (digitalRead(CONFIG_WIFI_PIN) == LOW) {
	*/

	if (drd.detectDoubleReset()) {

#ifdef LED0_PIN
		for(int i = 0; i < 16; i++) {
			digitalWrite(LED0_PIN, true);
			delay(125);
			digitalWrite(LED0_PIN, false);
			delay(125);
		}
		digitalWrite(LED0_PIN, false);
#endif

		Serial.println("Starting AP for reconfiguration ...");
		DRAWMESSAGE(display, "WIFI CONFIG!");

#ifndef ESP8266
		drawDisplay(&display, 0);
#endif

		WiFiManager wifiManager;
		//wifiManager.resetSettings();
		//wifiManager.startConfigPortal("ESPNODE");
		wifiManager.startConfigPortal();
	} else {

		isAP = true;
		Serial.println("Starting AP or connecting to Wi-Fi ...");

		while(!connectWiFi());
		/*
		for (int i = 0; i < 3; i++) {

#else
	if (true) {
		Serial.println("Starting connecting to Wi-Fi ...");
		yield();

		//TODO:
		for (int i = 0; i < 3; i++) {
#endif

			DRAWMESSAGE(display, "WIFI CONN... " + String(i));
			//if (wifiManager.autoConnect()) {
			if (wifiManager.connectWifi("", "")) {
				isAP = false;
				Serial.print("IP address: ");
				Serial.println(WiFi.localIP());
				DRAWMESSAGE(display, "WIFI CONN");
				break;
			}
			delay(1000);
			//Serial.println(wifiManager.getSSID());
			//Serial.println(wifiManager.getPassword());
		}
		if (isAP) {
			startWiFiAP();
		}

#ifdef LED1_PIN
		digitalWrite(LED1_PIN, HIGH);
#endif
	*/

	}

	Serial.println("READY");
	WiFi.printDiag(Serial);
	DRAWMESSAGE(display, "WIFI READY");

#ifdef MQTT
	mqttClient.setCallback(receivedCallback);
	//mqttLock.clear();
#endif

	//pinMode(INPUT1_PIN, INPUT); //, INPUT_PULLUP);
	//pinMode(RX_PIN, INPUT); //, INPUT_PULLUP);
	//attachInterrupt(digitalPinToInterrupt(INPUT0_PIN), handleInterruptPIN1, FALLING);
	//pinMode(TX_PIN, INPUT); //, INPUT_PULLUP);
	//attachInterrupt(digitalPinToInterrupt(INPUT1_PIN), handleInterruptTX, FALLING);

	httpServer.on("/",
			[]() {
				Serial.println("/");

#ifdef HTTP_AUTH
				if(!httpServer.authenticate(www_username, www_password))
				return httpServer.requestAuthentication();
#endif

				String message = htmlHeader;
				message += "<h1>STATION: ";
				message += talkbackApiKey;
				message += "</h1>";

#ifdef NTP
				time_t t = CE.toLocal(timeClient.getEpochTime());
				tmElements_t tm;
				breakTime(t, tm);

				message += "<h2>TIME: ";
				message +=String(int2string(tm.Hour) + ':' + int2string(tm.Minute) + ':' + int2string(tm.Second) + ' ' + int2string(tm.Year + 1970) + '-' + int2string(tm.Month) + '-' + int2string(tm.Day));
				message += "</h2>";
#endif

				message += "<hr>";
				if(isErrorConn)
					message += "<h2>SERVER CONNECTION ERROR</h2>";
				else
					message += "<h2>SERVER CONNECTION OK</h2>";


#ifdef SD_CS_PIN
			if(errorSD)
			message += "<h2>ALARM: SD CARD</h2>";
#endif

			//message += "<hr><h2>SETTINGS</h2>";
			for(int i = 0; i < DEVICES_NUM; i++) {
				message += "<hr>";
				message += "<h2>";
				message += "<a href=/dev?id=";
				message += i;
				message += ">";
				message += "ID ";
				//message += devices[i].par[3];
				message += char(i + 48);
				message += ": ";
				message += devices[i].name;
				message += "</a>";
				message += "<br>";

				message += "STATE: ";
				message += String(bitRead(devices[i].flags, OUTPUT_BIT));
				message += "<br>";
				//message += digitalRead(INPUT1_PIN);
				//message += "</h2>";
				message += "VALUE: ";
				message += devices[i].val;
				message += "</h2>";
				/*
				if(i == DEVICES_NUM - 2) {
					message += "TEMPERATURE [C]: ";
					message += temperature;
					message += "</h2>";
					//message += digitalRead(INPUT1_PIN);
					//message += "</h2>";
					message += "ALARM: ";
					message += String(bitRead(devices[i].flags, OUTPUT_BIT));
				}
				if(i == DEVICES_NUM - 1) {
					message += "HUMIDITY [%]: ";
					message += humidity;
					message += "</h2>";
					//message += digitalRead(INPUT1_PIN);
					//message += "</h2>";
					message += "ALARM: ";
					message += String(bitRead(devices[i].flags, OUTPUT_BIT));
				}
				*/

				/*
				if(i == 0) {
					message += "<hr><h3>VALVES</h3>";
				}
				if(i == DEV_ALARM) {
					message += "<hr><h3>LIMITS FLOW</h3>";
				}
				*/

				//if(i < DEV_ALARM) {
				//	message += char(i + 65);
				//	message += ": ";
				//}
				//message += devices[i].name;
				//if(bitRead(devices[i].flags, OUTPUT_BIT))
				//	message += " ON";
				//else
				//	message += " OFF";

			}
			//message += "<hr><h3>SYSTEM</h3>";

#ifdef SD_CS_PIN
			message += "<hr><a href=/logs>LOGS</a>";
#endif

			message += htmlFooter;
			httpServer.send(200, "text/html", message);
		});

#ifdef SD_CS_PIN
	httpServer.on("/log", []() {
		Serial.println("/log");
		Serial.println(httpServer.args());
		if(!httpServer.authenticate(www_username, www_password))
		return httpServer.requestAuthentication();
		if(httpServer.args() > 0)
		loadFromSdCard(httpServer.arg(0));
	});

	httpServer.on("/logs", [](){
		Serial.println("/logs");
		if(!httpServer.authenticate(www_username, www_password))
		  return httpServer.requestAuthentication();
		String message = htmlHeader;
		File root = SD.open("/");
		if(!root){
		  Serial.println("Failed to open directory");
		  return;
		}
		if(!root.isDirectory()){
		  Serial.println("Not a directory");
		  return;
		}

		message += "<table>";
		File file = root.openNextFile();
		while(file){
		  if(file.isDirectory()){
			  Serial.print("DIR: ");
			  Serial.println(file.name());
			  //if(levels){
			  //  listDir(fs, file.name(), levels -1);
		  }
		  else {
			  Serial.print("FILE: ");
			  Serial.print(file.name());
			  Serial.print("SIZE: ");
			  Serial.println(file.size());
			  message += "<tr><td><a href=/log?name=";
			  message += file.name();
			  message += ">";
			  message += file.name();
			  message += "</a></td><td>";
			  message += String(file.size());
			  message += "</td></tr>";
		  }
		  file = root.openNextFile();
		}
		message += "</table>";
		Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
		Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
		message += htmlFooter;
		httpServer.send(200, "text/html", message);
	});
#endif

	httpServer.on("/save", []() {
		Serial.println("/save");

#ifdef HTTP_AUTH
		if(!httpServer.authenticate(www_username, www_password))
		return httpServer.requestAuthentication();
#endif

		saveInstruments();
		char value;
		for(int i=0; i < 512; i++) {
			if(i % 32 == 0)
			Serial.println();
			value = EEPROM.read(i);
			Serial.print(value, HEX);
			Serial.print(' ');
		}
		String message = htmlHeader;
		message += "<h1>STATION: ";
		message += talkbackApiKey;
		message += "</h1><hr>";

		message += "OK";
		message += htmlFooter;
		httpServer.send(200, "text/html", message);
	});

	httpServer.on("/settings", []() {
		Serial.println("/settings");

#ifdef HTTP_AUTH
		if(!httpServer.authenticate(www_username, www_password))
			return httpServer.requestAuthentication();
#endif

		String message = htmlHeader;
		message += "<h1>STATION: ";
		message += talkbackApiKey;
		message += "</h1><hr>";

		message += "<h2>SYSTEM SETTINGS</h2><hr>";

#ifdef NTP
		time_t t = CE.toLocal(timeClient.getEpochTime());
		tmElements_t tm;
		breakTime(t, tm);

		message += "<form action=/savesettings>";
		message += "HOURS<br><input name=hour value=";
		message += tm.Hour;
		message += "><br>";
		message += "<br>MINUTES<br><input name=minute value=";
		message += tm.Minute;
		message += "><br>";
		message += "<br>SECONDS<br><input name=second value=";
		message += tm.Second;
		message += "><br>";
		message += "<br>YEAR<br><input name=year value=";
		message += tm.Year + 1970;
		message += "><br>";
		message += "<br>MONTH<br><input name=month value=";
		message += tm.Month;
		message += "><br>";
		message += "<br>DAY<br><input name=day value=";
		message += tm.Day;
		message += "><br><br>";
		message += "<button type=submit name=cmd value=settime>SET TIME</button>";
		message += "</form>";
		message += "<hr>";
#endif

		message += "<form action=/savesettings>";

#ifdef HTTP_AUTH
		message += "ADMIN PASSWORD<br><input name=www_password value=";
		message += www_password;
		message += "><hr>";
#endif

#ifdef THINGSSPEAK
		message += "SERVER<br><input name=servername value=";
		message += serverName;
		message += "><br>";
		//message += "<br>WRITE API KEY<br><input name=writeapikey value=";
		message += "<br>SCRIPT<br><input name=writeapikey value=";
		message += writeApiKey;
		message += "><br>";
		//message += "<br>TALKBACK ID<br><input name=talkbackid value=";
		message += "<br>PORT<br><input name=talkbackid value=";
		message += talkbackID;
		message += "><br>";
		//message += "<br>TALKBACK API KEY<br><input name=talkbackapikey value=";
		message += "<br>STATION NAME<br><input name=talkbackapikey value=";
		message += talkbackApiKey;
		message += "><hr>";
#endif

#ifdef MQTT
		message += "MQTT SERVER<br><input name=mqttserver value=";
		message += mqttServer;
		message += "><br>";
		message += "<br>MQTT USER<br><input name=mqttuser value=";
		message += mqttUser;
		message += "><br>";
		message += "<br>MQTT PASSWORD<br><input name=mqttpassword value=";
		message += mqttPassword;
		message += "><br>";
		message += "<br>MQTT ID<br><input name=mqttid value=";
		message += mqttID;
		message += "><br>";
		message += "<br>MQTT ROOT TOPIC (/)<br><input name=mqttroottopic value=";
		message += mqttRootTopic;
		message += "><hr>";
#endif

		message += "MODE!<br><input name=newMode value=";
		message += newMode;
		message += "><h3>";
		message += "ACTUAL MODE: ";
		message += mode;
		message += "</h3>";

		message += "1: IN PIN3 (RX) / OUT PIN2<br>";
		message += "2: ULTRASONIC ECHO PIN3 (RX) / TRIGG PIN2<br>";
		message += "3: DHT BUS PIN3 (RX) / OUT PIN2<br>";
		message += "4: COUNTER IN PIN3 (RX) / OUT PIN2<br>";
		message += "5: ONEWIRE BUS PIN3 (RX) / OUT PIN2<br>";
		message += "<hr>";

		message += "<button type=submit name=cmd value=setapi>SET API!</button>";
		message += "</form>";
		message += htmlFooter;
		httpServer.send(200, "text/html", message);
	});

	httpServer.on("/savesettings", []() {
		Serial.println("/savesettings");

#ifdef HTTP_AUTH
		if(!httpServer.authenticate(www_username, www_password))
		return httpServer.requestAuthentication();
#endif

		String message = htmlHeader;
		message += "<h1>STATION: ";
		message += talkbackApiKey;
		message += "</h1><hr>";

#ifdef NTP
		if(httpServer.arg("cmd").equals("settime")) {
			int offset = CE.toUTC(0) - CE.toLocal(0);
			Serial.print("Time offset: ");
			Serial.println(offset);

			tmElements_t tm;
			tm.Second = (unsigned long)httpServer.arg("second").toInt();
			tm.Minute = (unsigned long)httpServer.arg("minute").toInt();
			tm.Hour = (unsigned long)httpServer.arg("hour").toInt() + offset / 3600;;
			tm.Day = (unsigned long)httpServer.arg("day").toInt();
			tm.Month = (unsigned long)httpServer.arg("month").toInt();
			tm.Year = (unsigned long)httpServer.arg("year").toInt() - 1970;
			time_t t = makeTime(tm);// + offset;

			/*
			Serial.println("Old time:");
			Serial.println(timeClient.getEpochTime());
			Serial.println(timeClient.getFormattedTime());
			Serial.println("Set time:");
			Serial.println(t);
			Serial.println("New time:");
			Serial.println(timeClient.getEpochTime());
			Serial.println(timeClient.getFormattedTime());
			*/

			//timeClient.setEpochTime((h * 3600L + m * 60L + s) - timeClient.getEpochTime()) + offset);
			timeClient.setEpochTime(t);

			//Serial.println(h);
			//Serial.println(m);
			//Serial.println(s);
			//Serial.println(h * 3600 + m * 60 + s);
			message += "TIME SET";
		}
#endif
		if(httpServer.arg("cmd").equals("setapi")) {
			//TODO: disable for demo
			strcpy(www_password, httpServer.arg("www_password").c_str());

#ifdef THINGSSPEAK
			strcpy(serverName, httpServer.arg("servername").c_str());
			strcpy(writeApiKey, httpServer.arg("writeapikey").c_str());
			talkbackID = httpServer.arg("talkbackid").toInt();
			strcpy(talkbackApiKey, (char*)httpServer.arg("talkbackapikey").c_str());
#endif

#ifdef MQTT
			strcpy(mqttServer, (char*)httpServer.arg("mqttserver").c_str());
			strcpy(mqttUser, (char*)httpServer.arg("mqttuser").c_str());
			strcpy(mqttPassword, (char*)httpServer.arg("mqttpassword").c_str());
			mqttID = httpServer.arg("mqttid").toInt();
			strcpy(mqttRootTopic, (char*)httpServer.arg("mqttroottopic").c_str());
#endif
			newMode = httpServer.arg("newMode").toInt();

			message += "API SET";
			saveApi();
		}
		message += htmlFooter;
		httpServer.send(200, "text/html", message);
	});

	httpServer.on("/dev", []() {
		Serial.println("/dev");

#ifdef HTTP_AUTH
		if(!httpServer.authenticate(www_username, www_password))
			return httpServer.requestAuthentication();
#endif

		String cmd=httpServer.arg("cmd");
		Serial.println(cmd);
		byte id=httpServer.arg("id").toInt();
		if(cmd.equals("set")) {
			int par0=httpServer.arg("par0").toInt();
			Serial.println("par0");
			int par1=httpServer.arg("par1").toInt();
			Serial.println("par1");
			int par2=httpServer.arg("par2").toInt();
			Serial.println("par2");
			int par3=httpServer.arg("par3").toInt();
			Serial.println("par3");
			String name=httpServer.arg("name");
			Serial.println("name");
			if(id >=0 && id < DEVICES_NUM) {
				devices[id].par[0] = par0;
				devices[id].par[1] = par1;
				devices[id].par[2] = par2;
				devices[id].par[3] = par3;
				strncpy(devices[id].name, name.c_str(), 16);
			}
		}
		if(cmd.equals("auto")) {
			bitClear(devices[id].flags, MANUAL_BIT);
			bitClear(devices[id].flags, RUNONCE_BIT);
		}
		if(cmd.equals("off")) {
			bitSet(devices[id].flags, MANUAL_BIT);
			bitClear(devices[id].flags, CMD_BIT);
			bitClear(devices[id].flags, OUTPUT_BIT);
		}
		if(cmd.equals("on")) {
			bitSet(devices[id].flags, MANUAL_BIT);
			bitSet(devices[id].flags, CMD_BIT);
			bitSet(devices[id].flags, OUTPUT_BIT);
		}
		String message = htmlHeader;
		message += "<h1> STATION: ";
		message += talkbackApiKey;
		message += "</h1><hr>";

		message += getDeviceForm(id, devices);
		message += htmlFooter;
		httpServer.send(200, "text/html", message);
	});

#ifndef ESP8266
	//const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
	httpServer.on("/update", HTTP_GET, [](){
        if(!httpServer.authenticate(www_username, www_password))
        	return httpServer.requestAuthentication();
        httpServer.sendHeader("Connection", "close");
        String message = htmlHeader;
		message += serverIndex;
		message += htmlFooter;
		httpServer.send(200, "text/html", message);
	});

	httpServer.on("/update", HTTP_POST, [](){
        if(!httpServer.authenticate(www_username, www_password))
            return httpServer.requestAuthentication();
        httpServer.sendHeader("Connection", "close");\
        httpServer.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
        ESP.restart();
      },[](){
    	  HTTPUpload& upload = httpServer.upload();
    	  if(upload.status == UPLOAD_FILE_START){
    		  Serial.setDebugOutput(true);
    		  Serial.printf("Update: %s\n", upload.filename.c_str());
#ifdef ESP8266
    		  uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
#else
    		  uint32_t maxSketchSpace = 0x140000;//(ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
#endif
    		  if(!Update.begin(maxSketchSpace)){//start with max available size
    			  Update.printError(Serial);
    		  }
    	  }
    	  else if(upload.status == UPLOAD_FILE_WRITE){
    		  if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
    			  Update.printError(Serial);
    		  }
    	  } else if(upload.status == UPLOAD_FILE_END){
    		  if(Update.end(true)){ //true to set the size to the current progress
    			  Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    		  }
    		  else {
    			  Update.printError(Serial);
    		  }
    		  Serial.setDebugOutput(false);
    	  }
    	  yield();
      });
#endif

	httpServer.begin();
	//timeClient.update();
	MDNS.begin(host);

#ifdef ESP8266
	httpUpdater.setup(&httpServer, update_path);//, www_username, www_password);
#endif

	MDNS.addService("http", "tcp", 80);

	//ArduinoOTA.setPort(8266);
	ArduinoOTA.setHostname(host); //8266
	// No authentication by default
	//ArduinoOTA.setPassword((const char *)"xxxxx");
	ArduinoOTA.onStart([]() {
		Serial.println("OTA Start");
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("OTA End");
		Serial.println("Rebooting...");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();

#ifdef CONFIG_WIFI_PIN
	if (digitalRead(CONFIG_WIFI_PIN) == LOW) {
		//emergency restore OTA
		while (true) {
			ArduinoOTA.handle();
			httpServer.handleClient();
		}
	}
#endif

#ifndef ESP8266
	xTaskCreatePinnedToCore(loopComm, "loopComm", 4096, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
#endif
}

#ifndef ESP8266
void drawNextFrame(OLEDDisplay *display) {
	frameNo++;
	drawDisplay(display, frameNo);
}
#endif

String int2string(int i) {
	if (i < 10)
		return "0" + String(i);
	return String(i);
}

//TODO:
int makeHttpGet(int i) {
	String get = "http://" + String(serverName) + ":" + String(talkbackID) + "/" + writeApiKey + "?id=" + devices[i].par[3] + "&field1=" + devices[i].name + "&field2=";

	if(i < DEVICES_NUM)
		get += String(bitRead(devices[i].flags, OUTPUT_BIT)) + "&field3=" + String(counter[i]);
	/*
	if(i == DEVICES_NUM - 2)
		get += String(bitRead(devices[i].flags, OUTPUT_BIT)) + "&field3=" + String(temperature);
	if(i == DEVICES_NUM - 1)
		get += String(bitRead(devices[i].flags, OUTPUT_BIT)) + "&field3=" + String(humidity);
	 */

	Serial.println(get);
	DRAWMESSAGE(display, "TS CONN ...");

	//TODO:
	http.begin(get);
	httpCode = http.GET();
	Serial.println(httpCode);
	//Serial.println(http.errorToString(httpCode));
	DRAWMESSAGE(display, "TS DONE");
	if (httpCode != 200) {
		httpErrorCounter++;
		isErrorConn = true;
	} else {
		isErrorConn = false;
	}
	if (httpCode > 0) {
		String payload = http.getString();
		//Serial.println(payload);
	}
	http.end();
	return httpCode;
}

bool setAlarm(Device* pDevice) {
	//if(!bitRead(device.flags, OUTPUT_BIT))
	//	bitSet(device.flags, UNACK_BIT);
	//bitSet(device.flags, OUTPUT_BIT);
	if(!bitRead(pDevice->flags, OUTPUT_BIT))
		bitSet(pDevice->flags, UNACK_BIT);
	bitSet(pDevice->flags, OUTPUT_BIT);
}

bool clearAlarm(Device* pDevice) {
	//if(bitRead(device.flags, OUTPUT_BIT))
	//	bitSet(device.flags, UNACK_BIT);
	//bitClear(device.flags, OUTPUT_BIT);
	if(bitRead(pDevice->flags, OUTPUT_BIT))
		bitSet(pDevice->flags, UNACK_BIT);
	bitClear(pDevice->flags, OUTPUT_BIT);
}

////////////////////////
// communication loop
////////////////////////
void loopComm(void *pvParameters) {

#ifndef ESP8266
	while (1) {
#else
	if (1) {
#endif

		//TODO: output is true without this, why?
		if(mode == MODE_2_OUT_IN || mode == MODE_2_OUT_DHT22 || mode == MODE_2_OUT_COUNT || mode == MODE_2_OUT_ONEWIRE) {
#ifdef OUTPUT0_PIN
			digitalWrite(OUTPUT0_PIN, bitRead(devices[4].flags, OUTPUT_BIT));
#endif
		}

		//drawMessage(&display, String(millis()));
#ifdef NTP
		DRAWMESSAGE(display, "NTP CONN ...");
		timeClient.update();
		DRAWMESSAGE(display, "NTP DONE");
		//drawMessage(&display, "NTP DONE");
#endif

		isErrorConn = true;

#ifdef MQTT
		//if (!mqttLock.test_and_set() && mqttServer[0] != 0) {
		if (mqttServer[0] != 0) {
			DRAWMESSAGE(display, "MQTT CONN ...");
			/* this function will listen for incoming subscribed topic-process-invoke receivedCallback */
			mqttClient.loop();

			if (!mqttClient.connected()) {
				mqttConnect();
			}
			if (mqttClient.connected()) {
				for(int i = 0; i < DEVICES_NUM; i++) {
					String mqttTopic = String('/' + String(i));
					/*
					if(i == 0)
						mqttTopic += A_TOPIC;
					if(i == 1)
						mqttTopic += B_TOPIC;
					if(i == 2)
						mqttTopic += C_TOPIC;
					if(i == 3)
						mqttTopic += D_TOPIC;
					if(i == 4)
						mqttTopic += E_TOPIC;
					if(i == 5)
						mqttTopic += F_TOPIC;
					*/
					String mqttTopicState = String(mqttRootTopicState + mqttTopic);
					String mqttTopicVal = String(mqttRootTopicVal + mqttTopic);

					snprintf(msg, 20, "%d", bitRead(devices[i].flags, OUTPUT_BIT));
					mqttClient.publish(mqttTopicState.c_str(), msg);
					Serial.print(mqttTopicState);
					Serial.print(": ");
					Serial.println(msg);

					snprintf(msg, 20, "%f", devices[i].val);
					mqttClient.publish(mqttTopicVal.c_str(), msg);
					Serial.print(mqttTopicVal);
					Serial.print(": ");
					Serial.println(msg);
				}
				isErrorConn = false;
				DRAWMESSAGE(display, "MQTT DONE");
			}
			else {
				DRAWMESSAGE(display, "MQTT ERROR");
			}
			//mqttLock.clear();
		}
#endif

#ifdef SD_CS_PIN
		if(isSD) {
			DRAWMESSAGE(display, "SD LOG ...");
			int fileIndex = 0;
			time_t t = CE.toLocal(timeClient.getEpochTime());
			String path = "/" + String(year(t)) + "-" + int2string(month(t)) + "-" + int2string(day(t)) + ".csv";
			String message = String(year(t)) + "-" + int2string(month(t)) + "-" + int2string(day(t)) + " " + int2string(hour(t)) + ":" + int2string(minute(t)) + ":" + int2string(second(t)) + ";"
					+ String(bitRead(devices[DEV_ALARM_MAX].flags, OUTPUT_BIT) | bitRead(devices[DEV_ALARM_MIN].flags, OUTPUT_BIT)) + ";"
					+ String(level) + ";" + String(bitRead(devices[0].flags, OUTPUT_BIT)) + ";" + String(bitRead(devices[1].flags, OUTPUT_BIT)) + ";" + String(bitRead(devices[2].flags, OUTPUT_BIT)) + ";" + String(bitRead(devices[3].flags, OUTPUT_BIT)) + ";" + String(mqttState) + '\n';
			Serial.print(message);
			errorSD = false;
			if(!SD.exists(path)) {
				File file = SD.open(path, FILE_WRITE);
				if(file) {
					file.print("DATE TIME;ALARMS;LEVEL[mm];A: " + String(devices[0].name) + ";B: " + String(devices[1].name) +";C: " + String(devices[2].name) + ";D: " + String(devices[3].name) + ";MQTT\n");
					file.close();
				}
				else {
					Serial.println("Failed to create file");
					errorSD = true;
				}
			}
			File file = SD.open(path, FILE_APPEND);
			if(!file) {
				Serial.println("Failed to open file for appending");
				errorSD = true;
			}
			if(file) {
				if(file.print(message)){
					 //errorSD = false;
					 //Serial.println("Message appended");
				 }
				 else {
					 errorSD = true;
					 Serial.println("Failed to append file");
				 }
				 file.close();
			}
			if(errorSD)
				DRAWMESSAGE(display, "SD ERROR");
			else
				DRAWMESSAGE(display, "SD DONE");
		}
#endif

		/*
		if (!isAP) {
			if (!isCheckIn) {
				DRAWMESSAGE(display, "LOG CONN ...");
				http.begin("http://growmat.cz/growmatweb/dev/");
				httpCode = http.GET();
				if (httpCode > 0)
					isCheckIn = true;
			DRAWMESSAGE(display, "LOG DONE");
		}
		*/

#ifdef THINGSSPEAK
		if (serverName[0] != 0 && !isAP) {

#ifdef TALKBACK
			do {
#ifndef ESP8266
				drawMessage(&display, "CONN TB ...");
#endif

				String getTalkback = "http://" + String(serverName) + "/talkbacks/"
						+ String(talkbackID) + "/commands/execute?api_key="
						+ talkbackApiKey;
				Serial.println(getTalkback);

				//TODO:
				http.begin(getTalkback);
				httpCode = http.GET();
				Serial.println(httpCode);
				Serial.println(http.errorToString(httpCode));

#ifndef ESP8266
				drawMessage(&display, "TB DONE");
#endif

				if (httpCode != 200) {
						httpErrorCounter++;
						isErrorConn = true;
				} else {
					isErrorConn = false;
				}
				if (httpCode > 0) {
					String payload = http.getString();
					Serial.println(payload);
					if (payload == "")
						break;
					if (payload == "error_auth_required")
						break;
					if (payload.charAt(0) == 'A') {
						if (payload.charAt(1) == '0') {
							bitClear(devices[0].flags, OUTPUT_BIT);
							bitSet(devices[0].flags, MANUAL_BIT);
						} else if (payload.charAt(1) == '1') {
							bitSet(devices[0].flags, OUTPUT_BIT);
							bitSet(devices[0].flags, MANUAL_BIT);
						} else if (payload.charAt(1) == 'A') {
							bitClear(devices[0].flags, RUNONCE_BIT);
							bitClear(devices[0].flags, MANUAL_BIT);
						}
					}
					if (payload.charAt(0) == 'B') {
						if (payload.charAt(1) == '0') {
							bitClear(devices[1].flags, OUTPUT_BIT);
							bitSet(devices[1].flags, MANUAL_BIT);
						} else if (payload.charAt(1) == '1') {
							bitSet(devices[1].flags, OUTPUT_BIT);
							bitSet(devices[1].flags, MANUAL_BIT);
						} else if (payload.charAt(1) == 'A') {
							bitClear(devices[1].flags, RUNONCE_BIT);
							bitClear(devices[1].flags, MANUAL_BIT);
						}
					}
				} else
					break;
			} while (true);
#endif

			/*
			String alarm = String(bitRead(devices[3].flags, OUTPUT_BIT) * 1	+ bitRead(devices[4].flags, OUTPUT_BIT) * 10);
			String v1 = String(bitRead(devices[0].flags, OUTPUT_BIT) + bitRead(devices[0].flags, MANUAL_BIT) * 10);
			String v2 = String(bitRead(devices[1].flags, OUTPUT_BIT)+ bitRead(devices[1].flags, MANUAL_BIT) * 10);

			String get = "http://" + String(serverName) + "/update?key=" + writeApiKey
					+ "&field1=" + alarm + "&field2=" + String(level) + "&field3=" + v1
					+ "&field4=" + v2 + "&field5=" + millis() + "&field6="
					+ httpErrorCounter + "&field7=" + httpCode;
			*/
			//String get = "http://" + String(serverName) + "/update?key=" + writeApiKey
			//					+ "&field1="

			//TODO:
			makeHttpGet(deviceCommIndex);

			//for(int i = 0; i< DEVICES_NUM; i++) {
			//	makeHttpGet(i);
			//}

			/*
			String get = "http://" + String(serverName) + "/" + writeApiKey + "?id=" + talkbackID + "&field1=" + String(talkbackApiKey) + "&field2=" + String(counter) + "&field2=" + String(counter);
			Serial.println(get);
			DRAWMESSAGE(display, "TS CONN ...");
			http.begin(get);
			httpCode = http.GET();
			Serial.println(httpCode);
			Serial.println(http.errorToString(httpCode));
			DRAWMESSAGE(display, "TS DONE");
			if (httpCode != 200) {
				httpErrorCounter++;
				isErrorConn = true;
			} else {
				isErrorConn = false;
			}
			if (httpCode > 0) {
				String payload = http.getString();
				Serial.println(payload);
			}
			http.end();
			*/
		}
#endif

	}

#ifndef ESP8266
    //delay(26000);
	delay(56000);
#endif

    //if(isAP && reconnectTimeout > 10) {
	if (reconnectTimeout > 10) {
		reconnectTimeout = 0;
		//WiFi.softAPdisconnect(true);
		WiFi.mode(WIFI_STA);
		WiFi.begin();
		WiFi.mode(WIFI_STA);
		//WiFi.disconnect(true);
		WiFiManager wifiManager;
		//wifiManager.resetSettings();
		for (int i = 0; i < 3; i++) {
			DRAWMESSAGE(display, "WIFI CONN ... " + String(i));
			if (wifiManager.autoConnect()) {
			//if (wifiManager.connectWifi("", "")) {
				isAP = false;
				Serial.print("IP address: ");
				Serial.println(WiFi.localIP());
				DRAWMESSAGE(display, "CONN WIFI DONE");
				break;
				}
			}
		if (isAP) {
			DRAWMESSAGE(display, "CONN WIFI ERROR");
			startWiFiAP();
			isAP = true;
		} else
			reconnectTimeout++;
	}
}

/////////////////////////////////////
// loop
/////////////////////////////////////
void loop() {
	drd.loop();
	//mqttClient.loop();

	//devices[DEVICES_NUM - 1].val = millis();

	//bool alarm = bitRead(devices[DEV_ALARM].flags, OUTPUT_BIT);
	//bool unack = bitRead(devices[0].flags, UNACK_BIT);

#ifdef CONFIG_WIFI_PIN
	if (!digitalRead(CONFIG_WIFI_PIN)) {
		//bitClear(devices[DEV_ALARM].flags, UNACK_BIT);

#ifndef ESP8266
		display.normalDisplay();
#endif

	}
#endif

	// wdt test
	//ESP.wdtDisable(); //disable sw wdt, hw wdt keeps on
	//while(1){};

	ArduinoOTA.handle();
	httpServer.handleClient();
	//mqttClient.loop();

#ifdef LCD
	display.clearDisplay();
	display.setCursor(0,0);
	display.println(WiFi.localIP());
	display.println(timeClient.getFormattedTime());
	display.print(temperature);
	display.print('C');
	display.display();
#endif

	//if(minInterval.expired()) {
	//	minInterval.set(60000);
	//per minute
	//}


	if(mode == MODE_2_OUT_IN) {
#ifdef INPUT0_PIN
		devices[0].val = digitalRead(INPUT0_PIN);
		bitWrite(devices[0].flags, OUTPUT_BIT, digitalRead(INPUT0_PIN));
#endif
#ifdef INPUT1_PIN
		devices[1].val = digitalRead(INPUT1_PIN);
		bitWrite(devices[1].flags, OUTPUT_BIT, digitalRead(INPUT1_PIN));
#endif
#ifdef INPUT2_PIN
		devices[2].val = digitalRead(INPUT2_PIN);
		bitWrite(devices[2].flags, OUTPUT_BIT, digitalRead(INPUT2_PIN));
#endif
#ifdef INPUT3_PIN
		devices[3].val = digitalRead(INPUT3_PIN);
		bitWrite(devices[3].flags, OUTPUT_BIT, digitalRead(INPUT3_PIN));
#endif
	}

	if (millis() - lastMillis >= 1000) {
		/////////////////////////////////////
		// second loop
		/////////////////////////////////////

		connectWiFi();

#ifdef ESP8266
		if(secCounter % COMM_TIME == 0)
			loopComm(0);
#endif

#ifdef LED0_PIN
		digitalWrite(LED0_PIN, secCounter & 1);
#endif

		lastMillis = millis();
		secCounter++;

#ifdef INPUT0_PIN
		if(mode == MODE_2_US) {
			digitalWrite(OUTPUT0_PIN, LOW);
			delayMicroseconds(5);
			digitalWrite(OUTPUT0_PIN, HIGH);
			delayMicroseconds(10);
			digitalWrite(OUTPUT0_PIN, LOW);
			pinMode(INPUT0_PIN, INPUT);
			long d = pulseIn(INPUT0_PIN, HIGH);
			devices[0].val = (d/2) * 34.3; //[mm]

			if(devices[0].val > devices[0].par[0] || devices[0].val < devices[0].par[1])
				setAlarm(&devices[0]);
			else
				clearAlarm(&devices[0]);
		}
#endif

#ifdef DHT_PIN
		if(mode == MODE_2_OUT_DHT22) {
			float t = dht.readTemperature();
			//float t = pDHT->readTemperature();
			if(!isnan(t))
				devices[0].val = t;
			float h = dht.readHumidity();
			//float h = pDHT->readHumidity();
			if(!isnan(t))
				devices[1].val = h;
			for(int i = 0; i < 2; i++) {
				if(devices[i].val > devices[i].par[0] || devices[i].val < devices[i].par[1])
					setAlarm(&devices[i]);
				else
					clearAlarm(&devices[i]);
			}
		}
#endif

#ifdef ONEWIREBUS_PIN;
		if(mode == MODE_2_OUT_DHT22) {
			oneWireSensors.requestTemperatures();
			DeviceAddress tempDeviceAddress;
			for(int i = 0;  i < 4 && i < oneWireSensors.getDeviceCount(); i++) {
				oneWireSensors.getAddress(tempDeviceAddress, i);
				//oneWireSensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
				devices[i].val = oneWireSensors.getTempC(tempDeviceAddress);

				if(devices[i].val > devices[i].par[0] || devices[i].val < devices[i].par[1])
					setAlarm(&devices[i]);
				else
					clearAlarm(&devices[i]);
			}
		}
#endif

		if(mode == MODE_2_OUT_COUNT) {
			for(int i = 0; i < COUNTERS_NUM; i++) {
				devices[i].val = counter[i];

				counters[countersIndex][i] = counter[i];
				countersIndex %= COUNTERBUFFER_SIZE;

				uint8_t countersIndexCompare = countersIndex - devices[i].par[1];
				countersIndexCompare %= COUNTERBUFFER_SIZE;
				//Serial.print(counters[countersIndex][i] - counters[countersIndexCompare][i]);
				//Serial.println();

				if(counters[countersIndex][i] - counters[countersIndexCompare][i] > devices[i].par[0]) {
					setAlarm(&devices[i]);
				}
				else {
					clearAlarm(&devices[i]);
				}
				//Serial.println(devices[i].flags);
			}
			countersIndex++;
		}

		//uint8_t countersIndexCompare = countersIndex - devices[0].par1;
		//countersIndexCompare %= COUNTERBUFFER_SIZE;
		//countersIndexCompare = countersIndex - devices[0].par1;
		//if(countersIndexCompare < 0)
		//	countersIndexCompare += COUNTERBUFFER_SIZE;
		//Serial.print(countersIndexCompare);
		//Serial.print('\t');
		/*
		int j = 0;
		Serial.print(countersIndex);
		Serial.print('\t');
		Serial.println(counters[countersIndex][j]);
		Serial.print(countersIndexCompare);
		Serial.print('\t');
		Serial.println(counters[countersIndexCompare][j]);

			for(int i = 0; i < COUNTERBUFFER_SIZE; i++) {
			if(i == countersIndex)
				Serial.print('A');
			else if(i == countersIndexCompare)
				Serial.print('C');
			else
				Serial.print(' ');
			Serial.print(counters[i][j]);
		}
		Serial.println();
		*/

		for(int i = 4;  i < 8; i++) {
			if(bitRead(devices[i].flags, OUTPUT_BIT)) {
				outOnSecCounter[i - 4]++;
				devices[i].val++;
			}

			unsigned int onSec = devices[i].par[0] * 3600 + devices[i].par[1] * 60;
			unsigned int offSec = devices[i].par[2] * 60 + devices[i].par[3];

			time_t t = CE.toLocal(timeClient.getEpochTime());
			byte h = (t / 3600) % 24;
			byte m = (t / 60) % 60;
			byte s = t % 60;
			unsigned int sec = h * 3600 + m * 60;

			if(onSec) {
				if(onSec == sec) {
					Serial.println(String(i) + ": onTime");
					bitClear(devices[i].flags, MANUAL_BIT);
					bitSet(devices[i].flags, OUTPUT_BIT);
					devices[i].val = 0;
					outOnSecCounter[i - 4] = 0;
				}
			}

			if(offSec) {
				if(outOnSecCounter[i - 4] >= offSec) {
				//if(devices[i - 4].val > offSec) {
					Serial.println(String(i) + ": offTime");
					bitClear(devices[i].flags, MANUAL_BIT);
					bitClear(devices[i].flags, OUTPUT_BIT);
					outOnSecCounter[i - 4] = 0;
				}
			}
		}
/*
		for(int i = 0; i < 4; i++) {
			bool isAlarm = false;

			if(mode == MODE_2_OUT_COUNT) {
				if(i < COUNTERS_NUM) {
					uint8_t countersIndexCompare = countersIndex - devices[i].par[1];
					countersIndexCompare %= COUNTERBUFFER_SIZE;
*/
					/*
					Serial.println();
					Serial.println(i);
					Serial.print(countersIndexCompare);
					Serial.print('\t');
					Serial.println(counters[countersIndexCompare][i]);
					Serial.print(countersIndex);
					Serial.print('\t');
					Serial.println(counters[countersIndex][i]);
					for(int j = 0; j < COUNTERBUFFER_SIZE; j++) {
						if(j == countersIndex)
							Serial.print('A');
						else if(j == countersIndexCompare)
							Serial.print('C');
						else
							Serial.print(' ');
						Serial.print(counters[j][i]);
					}
					Serial.print("\tR");
					Serial.print(counters[countersIndex][i] - counters[countersIndexCompare][i]);
					Serial.println();
					*/
/*
					if(counters[countersIndex][i] - counters[countersIndexCompare][i] > devices[i].par[0]) {
						isAlarm= true;
					}
				}
			}

			if(isAlarm)
				setAlarm(devices[i]);
			else
				clearAlarm(devices[i]);
		}
*/
		//Serial.println(millis());
		for(int i = 0; i < DEVICES_NUM; i++) {

			//Serial.print(i);
			//Serial.print(':');
			//Serial.print(bitRead(devices[i].flags, OUTPUT_BIT));
			//Serial.println();

			if(bitRead(devices[i].flags, UNACK_BIT)) {
				if(serverName[0] ) {
					if(makeHttpGet(i) == 200) {
						//TODO:
					}
				}
				bitClear(devices[i].flags, UNACK_BIT);
			}
		}

#ifdef MQTT
		//if (!mqttLock.test_and_set()) {
		//}
		mqttClient.loop();
		//Serial.print("MQTT: ");
		//Serial.println(mqttClient.connected());
		isErrorConn = !mqttClient.connected();
		//mqttLock.clear();
#endif

#ifndef ESP8266
  		if(unack && alarm) {
  			if(secCounter % 2)
  				display.invertDisplay();
  			else
  				display.normalDisplay();
  		}
  		if(digitalRead(CONFIG_WIFI_PIN)==HIGH) {
  			frameNo++;
  		}
  		if(frameNo >= frameCount)
  			frameNo = 0;
  		drawDisplay(&display, frameNo);
#endif

#ifdef LED1_PIN
  		if (isAP) {
  			digitalWrite(LED1_PIN, led1);
  		}
#endif

//if ((secCounter & 0xF) == 0xF) {
//drawMessage(&display, "SENSORS ...");
//TODO:
//drawMessage(&display, "SENSORS DONE");
//}

#ifdef RFRX_PIN
		if (rcSwitch.available()) {
			Serial.print(rcSwitch.getReceivedValue());
			Serial.print('\t');
			Serial.print(rcSwitch.getReceivedBitlength());
			Serial.print('\t');
			Serial.print(rcSwitch.getReceivedDelay());
			Serial.print('\t');
			unsigned int* p = rcSwitch.getReceivedRawdata();
			for(int i = 0; i < RCSWITCH_MAX_CHANGES; i++)
				Serial.print(*(p + i));
			Serial.print('\t');
			Serial.print(rcSwitch.getReceivedProtocol());
		    rcSwitch.resetAvailable();
		    Serial.println();
		 }
#endif

#ifdef RFTX_PIN
		if ((secCounter & 0xF) == 0xF) {
  			//light protection
  			if(secCounter > devices[7].par0 || secOverflow || bitRead(devices[0].flags, MANUAL_BIT)) {
				if(bitRead(devices[0].flags, OUTPUT_BIT))
					//sendSignal(RFTX_PIN, rf1on);
					rcSwitch.send(r1On, 24);
				else
					//sendSignal(RFTX_PIN, rf1off);
					rcSwitch.send(r1Off, 24);
			}
			if(bitRead(devices[1].flags, OUTPUT_BIT))
				rcSwitch.send(r2On, 24);
			else
				rcSwitch.send(r2Off, 24);
			if(bitRead(devices[2].flags, OUTPUT_BIT))
				rcSwitch.send(r3On, 24);
			else
				rcSwitch.send(r3Off, 24);
		}
#endif

#ifdef LED0_PIN
//		digitalWrite(LED0_PIN, not (bitRead(devices[DEV_ALARM_MAX].flags, OUTPUT_BIT) | bitRead(devices[DEV_ALARM_MIN].flags, OUTPUT_BIT)));
#endif

		if(mode == MODE_2_OUT_IN || mode == MODE_2_OUT_DHT22 || mode == MODE_2_OUT_COUNT || mode == MODE_2_OUT_ONEWIRE) {
#ifdef OUTPUT0_PIN
			digitalWrite(OUTPUT0_PIN, bitRead(devices[4].flags, OUTPUT_BIT));
#endif
#ifdef OUTPUT1_PIN
			digitalWrite(OUTPUT1_PIN, not(bitRead(devices[5].flags, OUTPUT_BIT)));
#endif
#ifdef OUTPUT2_PIN
			digitalWrite(OUTPUT2_PIN, not(bitRead(devices[6].flags, OUTPUT_BIT)));
#endif
#ifdef OUTPUT3_PIN
			digitalWrite(OUTPUT3_PIN, not(bitRead(devices[7].flags, OUTPUT_BIT)));
#endif
		}

#ifdef MQTT
		//if (!mqttLock.test_and_set() && mqttServer[0] != 0) {
		if (mqttServer[0] != 0) {
			yield();

			bool isComm = false;
			for (int i = 0; i < DEVICES_NUM; i++) {
					if (bitRead(devices[i].flags, OUTPUT_BIT) != bitRead(devices[i].flags, PREVOUTPUT_BIT))
						isComm = true;
			}
			if(isComm) {

				DRAWMESSAGE(display, "MQTT CONN ...");
				mqttClient.loop();
				if (!mqttClient.connected()) {
					mqttConnect();
				}

				if (mqttClient.connected()) {
					for (int i = 0; i < DEVICES_NUM; i++) {
						if (bitRead(devices[i].flags,
								OUTPUT_BIT) != bitRead(devices[i].flags, PREVOUTPUT_BIT)) {
							if (bitRead(devices[i].flags, OUTPUT_BIT))
								bitSet(devices[i].flags, PREVOUTPUT_BIT);
							else
								bitClear(devices[i].flags, PREVOUTPUT_BIT);
							snprintf(msg, 20, "%d", bitRead(devices[i].flags, OUTPUT_BIT));

							String mqttTopic = String('/' + String(i));
							/*
							if(i == 0)
								mqttTopic += A_TOPIC;
							if(i == 1)
								mqttTopic += B_TOPIC;
							if(i == 2)
								mqttTopic += C_TOPIC;
							if(i == 3)
								mqttTopic += D_TOPIC;
							if(i == 4)
								mqttTopic += E_TOPIC;
							if(i == 5)
								mqttTopic += F_TOPIC;
							*/
							String mqttTopicState = String(mqttRootTopicState + mqttTopic);
							mqttClient.publish(mqttTopicState.c_str(), msg);
							Serial.print(mqttTopicState);
							Serial.print(": ");
							Serial.println(msg);

						}
					}
					DRAWMESSAGE(display, "MQTT DONE");
				}
				else
					DRAWMESSAGE(display, "MQTT ERROR");
			}
			//mqttLock.clear();
		}
	}
#endif
}


