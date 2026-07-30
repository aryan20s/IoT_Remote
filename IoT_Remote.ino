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


void ipChooserScreen(SSD1306Wire* dsp, bool isHome) {
    IPAddress local = WiFi.localIP();
    uint8_t ip[4] = {local[0], local[1], local[2], local[3]};

    int digits[12];
    for (int i = 0; i < 4; i++) {
        digits[i * 3]     = ip[i] / 100;
        digits[i * 3 + 1] = (ip[i] / 10) % 10;
        digits[i * 3 + 2] = ip[i] % 10;
    }

    int curDigit = 0;
    bool prev_l = false, prev_r = false, prev_a = false, prev_b = false;
    
    while (true) {
        dsp->clear();
        dsp->drawString(0, 0, "Set MQTT IP:");
        
        int x = 6;
        for (int i = 0; i < 12; i++) {
            if (i == curDigit) {
                dsp->setColor(INVERSE);
                dsp->fillRect(x, 16, 7, 12);
                dsp->drawString(x + 1, 16, String(digits[i]));
                dsp->setColor(WHITE);
            } else {
                dsp->drawString(x + 1, 16, String(digits[i]));
            }
            x += 7;
            
            if (i % 3 == 2 && i < 11) {
                dsp->drawString(x, 16, ".");
                x += 6;
            }
        }
        
        dsp->drawString(0, 36, "L/R: Select  A/B: +/-");
        dsp->drawString(0, 48, "Hold L+R to confirm");
        dsp->display();

        bool cur_l = digitalRead(INP_L_PIN) == HIGH;
        bool cur_r = digitalRead(INP_R_PIN) == HIGH;
        bool cur_a = digitalRead(INP_A_PIN) == HIGH;
        bool cur_b = digitalRead(INP_B_PIN) == HIGH;

        if (cur_l && cur_r) {
            break;
        }

        bool l = !cur_l && prev_l;
        bool r = !cur_r && prev_r;
        bool a = !cur_a && prev_a;
        bool b = !cur_b && prev_b;

        if (l) { curDigit = (curDigit - 1 + 12) % 12; }
        if (r) { curDigit = (curDigit + 1) % 12; }
        
        if (a) {
            digits[curDigit] = (digits[curDigit] + 1) % 10;
        }
        if (b) {
            digits[curDigit] = (digits[curDigit] - 1 + 10) % 10;
        }

        prev_l = cur_l;
        prev_r = cur_r;
        prev_a = cur_a;
        prev_b = cur_b;
        
        delay(20);
    }
    
    for (int i = 0; i < 4; i++) {
        int val = digits[i * 3] * 100 + digits[i * 3 + 1] * 10 + digits[i * 3 + 2];
        if (val > 255) val = 255;
        ip[i] = val;
    }

    String ipStr = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    
    dsp->clear();
    dsp->drawString(0, 0, "Using IP:");
    dsp->drawString(0, 12, ipStr);
    dsp->display();
    
    net_setServerIP(ipStr.c_str());
}

void setup() {
    Serial.begin(115200);
    updateDHT();
    disp_init();
    auto *dsp = disp_getDisplay();

    dsp->flipScreenVertically();
    dsp->clear();
    dsp->setFont(ArialMT_Plain_10);

    dsp->drawString(0, 0, "L: alt net | R: learn");
    dsp->drawString(0, 12, "A: raw IR");
    dsp->drawString(0, 24, "Continuing in 2 sec...");
    dsp->display();

    unsigned long timeStart = millis();
    bool homeWifi = false;
    bool irLearn = false;
    bool irRaw = false;
    while ((millis() - timeStart) < 2000) {
        homeWifi = digitalRead(INP_L_PIN) == HIGH;
        irLearn  = digitalRead(INP_R_PIN) == HIGH;
        irRaw    = digitalRead(INP_A_PIN) == HIGH;
        if (homeWifi || irLearn || irRaw) { break; }
        delay(10);
    }

    if (irLearn) {
        dsp->clear();
        dsp->drawString(0, 0, "IR Learning Mode");
        dsp->drawString(0, 12, "Open serial monitor.");
        dsp->display();
        IR_recv_loop(); // never returns
    }
    
    if (irRaw) {
        dsp->clear();
        dsp->drawString(0, 0, "Raw IR Mode");
        dsp->drawString(0, 12, "Open serial monitor.");
        dsp->display();
        IR_raw_loop(); // never returns
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

    ipChooserScreen(dsp, homeWifi);

    delay(1000);

    dsp->clear();
    dsp->drawString(0, 0, "Connecting to MQTT...");
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
