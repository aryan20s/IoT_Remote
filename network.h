#ifndef NETWORK_H
#define NETWORK_H

#include <PubSubClient.h>

enum MsgType {
    S2C_SET_STATE,
    C2S_UPD_STATE,
};

void net_init(const char* ssid, const char* password, const char* mqttServer, MQTT_CALLBACK_SIGNATURE);
void net_sendUpdatePacket(int temp, bool power, float roomTemp, float humidity);
void net_connectMQTT();
PubSubClient* net_getClient();

#endif