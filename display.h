#ifndef DISPLAY_H
#define DISPLAY_H

#include <SSD1306Wire.h>
#include "ir.h"

struct ACState {
    int temp;
    int fanSpd;
    bool power;
};

void disp_init();
void disp_update(float roomTemp, float humidity);
SSD1306Wire* disp_getDisplay();

void disp_setTempPower(int temp, bool power);
ACState disp_getCurState();
Remote disp_getCurRemote();

#endif
