#include <Arduino.h>

#define INP_L_PIN 16
#define INP_R_PIN 0
#define INP_A_PIN 15
#define INP_B_PIN 2

void resetPins() {
    digitalWrite(INP_L_PIN, 0);
    digitalWrite(INP_R_PIN, 0);
    digitalWrite(INP_A_PIN, 0);
    digitalWrite(INP_B_PIN, 0);
}

uint32_t reverse32bit(uint32_t x) {
    x = ((x >> 1) & 0x55555555u) | ((x & 0x55555555u) << 1);
    x = ((x >> 2) & 0x33333333u) | ((x & 0x33333333u) << 2);
    x = ((x >> 4) & 0x0f0f0f0fu) | ((x & 0x0f0f0f0fu) << 4);
    x = ((x >> 8) & 0x00ff00ffu) | ((x & 0x00ff00ffu) << 8);
    x = ((x >> 16) & 0xffffu) | ((x & 0xffffu) << 16);
    return x;
}

int clamp(int value, int min, int max) {
    if (value < min) { return min; }
    if (value > max) { return max; }
    return value;
}

void printData(uint32_t num, int bits = 32) {
    for (int i = bits - 1; i >= 0; i--) {
        Serial.print((num & (1 << i)) ? '1': '0');
    }
}