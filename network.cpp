#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "network.h"

#define MSG_BUFFER_SIZE	(50)
#define CLIENT_ID "EC170E0A"

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];

String clientID = String(CLIENT_ID);
String sendChID = String("C2S/") + String(CLIENT_ID);
String recvChID = String("S2C/") + String(CLIENT_ID);

void net_init(const char* ssid, const char* password, const char* mqttServer, MQTT_CALLBACK_SIGNATURE) {
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    client.setServer(mqttServer, 1883);
    client.setCallback(callback);
}

void net_sendUpdatePacket(int temp, bool power, float roomTemp, float humidity) {
    JsonDocument doc;
    doc["msg"] = C2S_UPD_STATE;
    doc["temp"] = temp;
    doc["power"] = power;
    doc["roomTemp"] = roomTemp;
    doc["humidity"] = humidity;
    String toSend;
    serializeJson(doc, toSend);
    
    client.publish(sendChID.c_str(), toSend.c_str());
    Serial.print("Updating server on ");
    Serial.print(sendChID.c_str());
    Serial.print(": ");
    Serial.println(toSend.c_str());
}

void net_connectMQTT() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        
        if (client.connect(clientID.c_str())) {
            Serial.println("Connected");
            client.subscribe(recvChID.c_str());
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(". Retrying...");
            delay(1000);
        }
        yield();
    }
}

PubSubClient* net_getClient() {
    return &client;
}