#ifndef IR_H
#define IR_H

#include <Arduino.h>

enum Remote {
    LG_AKB7 = 0,
    MITSUB = 1,
    DAIKIN = 2
};

void IR_init();
void IR_send(uint8_t temperature, uint8_t fanSpeed, bool powerOff, Remote type);
void IR_recv_loop();
void IR_raw_loop();

#endif