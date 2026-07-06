#include <ArduinoJson.h>
#include <WiFi.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include "pins.h"
#include "mqtt_net.h"
#include "ir.h"
#include "utils.h"
#include "display.h"

#define DHTTYPE    DHT11 

ACState last;
unsigned long lastTimeDataSent;
unsigned long lastTimeTempRead;
float curRoomTemp = 0, curHumidity = 0;
DHT_Unified dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200);
    updateDHT();
    disp_init();
    auto *dsp = disp_getDisplay();

    dsp->flipScreenVertically();
    dsp->clear();
    dsp->setFont(ArialMT_Plain_10);

    dsp->drawString(0, 0, "L: alt network | R: IR learn");
    dsp->drawString(0, 12, "Continuing in 2 seconds...");
    dsp->display();

    unsigned long timeStart = millis();
    bool homeWifi = false;
    bool irLearn = false;
    while ((millis() - timeStart) < 2000) {
        homeWifi = digitalRead(INP_L_PIN);
        irLearn  = digitalRead(INP_R_PIN);
        if (homeWifi || irLearn) { break; }
        delay(10);
    }

    if (irLearn) {
        dsp->clear();
        dsp->drawString(0, 0, "IR Learning Mode");
        dsp->drawString(0, 12, "Open serial monitor.");
        dsp->display();
        IR_recv_loop(); // never returns
    }

    dsp->clear();
    dsp->drawString(0, 0, "Connecting to WiFi");
    dsp->display();

    IR_init();
    if (homeWifi) {
        net_init("TP-Link_90B6", "15476517", "192.168.0.102", mqttCallback);
    } else {
        net_init("vivo V23e 5G", "aryan234", "10.35.30.155", mqttCallback);
    }
    
    int count = 0;
    while (WiFi.status() != WL_CONNECTED) {
        count++;

        if (count == 4) {
            count = 0;
            dsp->clear();
            dsp->drawString(0, 0, "Connecting to WiFi");
        } else {
            dsp->drawString(count * 5, 12, ".");
        }

        Serial.print(".");
        dsp->display();
        delay(500);
    }

    dsp->drawString(0, 24, "WiFi connected");
    dsp->drawString(0, 36, "IP: " + WiFi.localIP().toString());
    dsp->display();

    Serial.println("\nWiFi connected");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());

    delay(1000);

    dsp->drawString(0, 48, "Connecting to MQTT...");
    dsp->display();
    net_connectMQTT();
    dsp->clear();
    dsp->drawString(0, 0, "MQTT connected.");
    dsp->display();

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
        IR_send(state.temp, state.fanSpd, state.power, disp_getCurRemote());

        net_sendUpdatePacket(state.temp, state.power, curRoomTemp, curHumidity);
    }

    PubSubClient* client = net_getClient();
    if (!client->connected()) {
        auto *dsp = disp_getDisplay();
        dsp->clear();
        dsp->drawString(0, 0, "MQTT connection lost.");
        dsp->drawString(0, 12, "Reconnecting...");
        dsp->display();

        net_connectMQTT();
        dsp->drawString(0, 24, "Connected.");
        dsp->display();
        delay(1000);
    }
    client->loop();

    unsigned long curTime = millis();
    if ((curTime - lastTimeDataSent) > 1000) {
        net_sendUpdatePacket(state.temp, state.power, curRoomTemp, curHumidity);
        lastTimeDataSent = curTime;
    }

    if ((curTime - lastTimeTempRead) > 2000) {
        updateDHT();
        lastTimeTempRead = curTime;
    }

    delay(50);
}
