#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <U8g2lib.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include "network.h"
#include "ir.h"
#include "utils.h"
#include "display.h"

#define DHTPIN 13
#define INP_L_PIN 16
#define DHTTYPE    DHT11 

ACState last;
int lastTimeDataSent;
int lastTimeTempRead;
float curRoomTemp = 0, curHumidity = 0;
DHT_Unified dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200);
    updateDHT();
    disp_init();
    auto *u8g2 = disp_getu8g2();

    resetPins();
    u8g2->clearBuffer();
    u8g2->setFont(u8g2_font_5x7_tr);
    
    u8g2->setCursor(0, 8);
    u8g2->print("Press L for alt. network");
    u8g2->setCursor(0, 16);
    u8g2->print("Continuing in 2 seconds...");
    u8g2->sendBuffer();

    int timeStart = millis();
    bool homeWifi = false;
    while ((millis() - timeStart) < 2000) {
        resetPins();
        homeWifi  = !digitalRead(INP_L_PIN);
        if (homeWifi) { break; }
        yield();
    }

    u8g2->clearBuffer();
    u8g2->setCursor(0, 8);
    u8g2->print("Connecting to WiFi");
    u8g2->sendBuffer();

    IR_init();
    if (homeWifi) {
        net_init("TP-Link_90B6", "15476517", "192.168.0.102", mqttCallback);
    } else {
        net_init("vivo V23e 5G", "aryan234", "192.168.154.155", mqttCallback);
    }
    
    int count = 0;
    while (WiFi.status() != WL_CONNECTED) {
        count++;

        if (count == 4) {
            count = 0;
            u8g2->clearBuffer();
            u8g2->setCursor(0, 8);
            u8g2->print("Connecting to WiFi");
        } else {
            u8g2->print(".");
        }

        Serial.print(".");
        u8g2->sendBuffer();
        yield();
    }

    u8g2->setCursor(0, 16);
    u8g2->print("WiFi connected");
    u8g2->setCursor(0, 24);
    u8g2->print("Local IP: ");
    u8g2->print(WiFi.localIP());
    u8g2->sendBuffer();

    Serial.println("\nWiFi connected");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());

    delay(1000);

    u8g2->setCursor(0, 40);
    u8g2->print("Connecting to MQTT...");
    u8g2->sendBuffer();
    net_connectMQTT();
    u8g2->setCursor(0, 48);
    u8g2->print("Connected.");
    u8g2->sendBuffer();

    delay(2500);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    JsonDocument doc;
    deserializeJson(doc, (char*) payload);
    serializeJson(doc, Serial);
    Serial.println();
    int msg = doc["msg"];

    switch (msg) {
        case S2C_SET_STATE: {
            if (doc["temp"].is<int>() && doc["power"].is<bool>()) {
                int temp = doc["temp"];
                bool pow = doc["power"];
                disp_setTempPower(temp, pow);
            }
            break;
        }
        default:
            Serial.print("Unknown packet type: ");
            Serial.println(msg);
            break;
    }
}

void updateDHT() {
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (!isnan(event.temperature)) {
        curRoomTemp = event.temperature;
    }
    
    dht.humidity().getEvent(&event);
    if (!isnan(event.relative_humidity)) {
        curHumidity = event.relative_humidity;
    }
}

void loop() {
    disp_update(curRoomTemp, curHumidity);

    ACState state = disp_getCurState();
    if ((state.fanSpd != last.fanSpd) || (state.temp != last.temp) || (state.power != last.power)) {
        last = state;
        resetPins();
        IR_send(state.temp, state.fanSpd, state.power, disp_getCurRemote());
        resetPins();

        net_sendUpdatePacket(state.temp, state.power, curRoomTemp, curHumidity);
    }

    PubSubClient* client = net_getClient();
    if (!client->connected()) {
        auto *u8g2 = disp_getu8g2();
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_5x7_tr);
        u8g2->setCursor(0, 8);
        u8g2->print("MQTT connection lost.");
        u8g2->setCursor(0, 16);
        u8g2->print("Reconnecting to MQTT...");
        u8g2->sendBuffer();

        net_connectMQTT();
        u8g2->setCursor(0, 24);
        u8g2->print("Connected.");
        u8g2->sendBuffer();
        delay(1000);
    }
    client->loop();

    int curTime = millis();
    if ((curTime - lastTimeDataSent) > 1000) {
        net_sendUpdatePacket(state.temp, state.power, curRoomTemp, curHumidity);
        lastTimeDataSent = curTime;
    }

    if ((curTime - lastTimeTempRead) > 2000) {
        //updateDHT();
        curRoomTemp = 27.5;
        curHumidity = 65;
        lastTimeTempRead = curTime;
    }
}
