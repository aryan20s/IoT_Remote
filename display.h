#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>

struct ACState {
    int temp;
    int fanSpd;
    bool power;
};

void disp_init();
void disp_update(float roomTemp, float humidity);
U8G2_SSD1306_128X64_NONAME_F_SW_I2C* disp_getu8g2();

void disp_setTempPower(int temp, bool power);
ACState disp_getCurState();
Remote disp_getCurRemote();

#endif
