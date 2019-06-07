#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2019-06-07 19:15:13

#include "Arduino.h"
#include "Arduino.h"
#define MODE_2_INOUT	1
#define MODE_2_US		2
#define MODE_2_DHT22	3
#define MODE_2_COUNT	4
extern int mode;
extern int newMode;
#define CONFIG_WIFI_PIN 0
#define INPUT0_PIN 3
#define OUTPUT0_PIN 2
#include "libraries/DoubleResetDetector.h"
#define DRD_TIMEOUT 10
#define DRD_ADDRESS 0
extern DoubleResetDetector drd;
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include "ESP8266HTTPClient.h"
#include <ESP8266HTTPUpdateServer.h>
#include "libraries/WiFiManager.h"
extern ESP8266WebServer httpServer;
extern ESP8266HTTPUpdateServer httpUpdater;
#define DRAWMESSAGE(display, message) (drawMessage(message))
#include <WiFiClient.h>
#include <DNSServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#define DEVICES_NUM 6
extern WiFiClient espClient;
extern IPAddress deviceIP;
extern bool isAP;
extern bool isNoComm[];
extern volatile int deviceCommIndex;
extern bool isCheckIn;
extern bool isErrorConn;
extern int reconnectTimeout;
#define COUNTERS_NUM  1
#define COUNTERBUFFER_SIZE 64
extern unsigned long counter[];
extern uint8_t countersIndex;
#define DHT_PIN 2
#define DHT11 11
#define DHT22 22
#define DHT21 21
#include "libraries/DHT.h"
extern DHT* pDHT;
#define MQTT
#include <PubSubClient.h>
extern PubSubClient mqttClient;
extern String mqttRootTopicVal;
extern String mqttRootTopicState;
extern int mqttState;
#define OUTPUT_BIT 0
#define MANUAL_BIT 1
#define CMD_BIT 2
#define UNACK_BIT 3
#define RUNONCE_BIT 4
#define PREVOUTPUT_BIT 5
#define MQTT_CLIENTID   "GROWMAT-"
#define ROOT_TOPIC		"GROWMAT/"
#define A_TOPIC 	"/A"
#define B_TOPIC 	"/B"
#define C_TOPIC 	"/C"
#define D_TOPIC 	"/D"
#define E_TOPIC 	"/E"
#define F_TOPIC 	"/F"
extern char msg[];
#define ARDUINO_RUNNING_CORE 1
extern const char* host;
extern const char* update_path;
extern char* htmlHeader;
extern char* htmlHeader;
extern char* htmlFooter;
extern char* htmlFooter;
extern const char* www_username;
extern char www_password[];
#define THINGSSPEAK
extern char serverName[];
extern char writeApiKey[];
extern unsigned int talkbackID;
extern char talkbackApiKey[];
extern char mqttServer[];
extern char mqttUser[];
extern char mqttPassword[];
extern unsigned int mqttID;
extern String mqttClientId;
extern HTTPClient http;
extern int httpCode;
extern unsigned int httpErrorCounter;

void drawMessage(String msg) ;
void receivedCallback(char* topic, byte* payload, unsigned int length) ;
void mqttConnect() ;
String getDeviceForm(int i, struct Device devices[]) ;
void handleRoot() ;
void saveApi() ;
void saveInstruments() ;
void startWiFiAP() ;
void handleInterrupt0() ;
void handleInterrupt1() ;
void handleInterrupt2() ;
void handleInterrupt3() ;
bool connectWiFi() ;
void setup() ;
String int2string(int i) ;
int makeHttpGet(int i) ;
void loopComm(void *pvParameters) ;
void loop() ;

#include "espnode.ino"


#endif
