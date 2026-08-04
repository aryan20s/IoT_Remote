#include "pins.h"

#define ELEMENT_UI_SIZE 32
#define ELEMENT_ICON_SIZE 28
#define ELEMENTS_Y_OFFSET 12

#include <Arduino.h>
#include <SSD1306Wire.h>

#include "ir.h"
#include "utils.h"
#include "display.h"

SSD1306Wire oled(0x3C, OLED_SDA, OLED_SCL, GEOMETRY_128_64, I2C_ONE, 1000000);

static unsigned char Selection_Icon[] = {
   0x1f, 0xf0, 0x0f, 0xf8, 0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x80,
   0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x80,
   0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x80,
   0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x80,
   0x01, 0x00, 0x00, 0x80, 0x1f, 0xf0, 0x0f, 0xf8 
};

static unsigned char Settings_Icon[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x07, 0xfc, 0xdf, 0x2f,
   0xfe, 0xdf, 0x6f, 0xfe, 0xdf, 0x6f, 0xfc, 0xdf, 0x2f, 0x00, 0x80, 0x07,
   0x00, 0x00, 0x00, 0xe0, 0x01, 0x00, 0xf4, 0xfb, 0x3f, 0xf6, 0xfb, 0x7f,
   0xf6, 0xfb, 0x7f, 0xf4, 0xfb, 0x3f, 0xe0, 0x01, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x80, 0x07, 0xfc, 0xdf, 0x2f, 0xfe, 0xdf, 0x6f, 0xfe, 0xdf, 0x6f,
   0xfc, 0xdf, 0x2f, 0x00, 0x80, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

static unsigned char Remote_Icon[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03,
   0xe0, 0xff, 0x07, 0x20, 0x00, 0x04, 0xa0, 0xff, 0x05, 0xa0, 0x00, 0x05,
   0xa0, 0x00, 0x05, 0xa0, 0x00, 0x05, 0xa0, 0x00, 0x05, 0xa0, 0x00, 0x05,
   0xa0, 0xff, 0x05, 0x20, 0x00, 0x04, 0xa0, 0x8d, 0x04, 0xa0, 0xcd, 0x05,
   0x20, 0x80, 0x04, 0xa0, 0x0d, 0x04, 0xa0, 0xcd, 0x05, 0x20, 0xc0, 0x05,
   0xa0, 0xcf, 0x05, 0x20, 0x00, 0x04, 0xe0, 0xff, 0x07, 0x00, 0x00, 0x00
};

static unsigned char Exit_Icon[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x1f, 0x00,
   0x1c, 0x10, 0x00, 0x34, 0x10, 0x00, 0x64, 0x90, 0x00, 0xc4, 0xd0, 0x00,
   0x84, 0xf0, 0x00, 0x84, 0xf0, 0x3f, 0x84, 0xf8, 0x3f, 0x84, 0xfc, 0x3f,
   0x84, 0xfe, 0x3f, 0xa4, 0xfc, 0x3f, 0x84, 0xf8, 0x3f, 0x84, 0xf0, 0x3f,
   0x84, 0xf0, 0x00, 0x84, 0xd0, 0x00, 0x84, 0x90, 0x00, 0x84, 0x10, 0x00,
   0xfc, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

enum UIState {
    IN_MAIN,
    IN_SETTINGS
};

enum Element {
    TEMPERATURE = 0,
    FAN = 1,
    POWER = 2,
    SETTINGS = 3,
    LG_AKB7REM = 4,
    MITSUBREM = 5,
    DAIKINREM = 6,
    EXIT = 7
};

const char* UIStrings[] = {
    "Temperature: Use A and B",
    "to change up or down.",
    "Fan Speed: Use A and B",
    "to change up or down.",
    "Power: Use A or B",
    "to toggle.",
    "Settings: Choose the",
    "remote model to use.",
    "LG_AKB7",
    "",
    "MITSUB_ELEC",
    "",
    "DAIKIN",
    "",
    "Return to main menu.",
    "",
};

UIState curUIState = IN_MAIN;
int curSelectedElem = TEMPERATURE;
ACState curState;
Remote curRemote = MITSUB;

void disp_init() {
    curState.temp = 24;
    curState.fanSpd = 3;
    curState.power = false;

    oled.init();
    oled.setContrast(255);
    oled.setFont(ArialMT_Plain_10);

    pinMode(INP_L_PIN, INPUT);
    pinMode(INP_R_PIN, INPUT);
    pinMode(INP_A_PIN, INPUT);
    pinMode(INP_B_PIN, INPUT);

    oled.clear();
    oled.display();
}

SSD1306Wire* disp_getDisplay() {
    return &oled;
}

void drawElement(int element, int x, int y) {
    oled.setColor(WHITE);
    oled.drawRect(x++, y++, ELEMENT_ICON_SIZE, ELEMENT_ICON_SIZE);
    oled.drawRect(x++, y++, ELEMENT_ICON_SIZE - 2, ELEMENT_ICON_SIZE - 2);

    switch (element) {
        case TEMPERATURE: {
            String disp = String(curState.temp) + "C";
            oled.drawString(x + 2, y + (ELEMENT_ICON_SIZE / 2) - 7, disp);
            break;
        }
        case FAN: {
            for (int i = 0; i < curState.fanSpd; i++) {
                oled.fillRect(x + (i * 8) + 1, y + 1, 6, 22);
            }
            break;
        }
        case POWER: {
            oled.drawString(x + 2, y + (ELEMENT_ICON_SIZE / 2) - 7, curState.power ? "ON" : "OFF");
            break;
        }
        case SETTINGS: {
            oled.drawXbm(x, y, 24, 24, Settings_Icon);
            break;
        }
        case LG_AKB7REM: case MITSUBREM: case DAIKINREM: {
            oled.drawXbm(x, y, 24, 24, Remote_Icon);
            break;
        }
        case EXIT: {
            oled.drawXbm(x, y, 24, 24, Exit_Icon);
            break;
        }
    }
}

void drawCenteredUI(const char* line1, const char* line2, const char* topLine) {
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(64, ELEMENTS_Y_OFFSET + ELEMENT_UI_SIZE + 1, line1);
    if (strlen(line2) > 0) {
        oled.drawString(64, ELEMENTS_Y_OFFSET + ELEMENT_UI_SIZE + 12, line2);
    }
    oled.drawString(64, 0, topLine);
    oled.setTextAlignment(TEXT_ALIGN_LEFT);
}

void disp_update(float roomTemp, float humidity) {
    oled.clear();

    static bool prev_l = false, prev_r = false, prev_a = false, prev_b = false;
    bool cur_l = digitalRead(INP_L_PIN) == HIGH;
    bool cur_r = digitalRead(INP_R_PIN) == HIGH;
    bool cur_a = digitalRead(INP_A_PIN) == HIGH;
    bool cur_b = digitalRead(INP_B_PIN) == HIGH;

    bool left_btn  = !cur_l && prev_l;
    bool right_btn = !cur_r && prev_r;
    bool a_btn     = !cur_a && prev_a;
    bool b_btn     = !cur_b && prev_b;

    prev_l = cur_l; prev_r = cur_r; prev_a = cur_a; prev_b = cur_b;

    switch (curUIState) {
        case IN_MAIN: {
            if ((left_btn + right_btn + a_btn + b_btn) == 1) { // only one button or else ignore
                if (left_btn) {
                    curSelectedElem--;
                } else if (right_btn) {
                    curSelectedElem++;
                } else {
                    switch (curSelectedElem) {
                        case TEMPERATURE: {
                            if (a_btn) {
                                curState.temp++;
                            } else {
                                curState.temp--;
                            }

                            curState.temp = clamp(curState.temp, 18, 30);
                            break;
                        }
                        case FAN: {
                            if (a_btn) {
                                curState.fanSpd++;
                            } else {
                                curState.fanSpd--;
                            }

                            curState.fanSpd = clamp(curState.fanSpd, 0, 3);
                            break;
                        }
                        case POWER: {
                            if (a_btn || b_btn) {
                                curState.power = !curState.power;
                            } 
                            break;
                        }
                        case SETTINGS: {
                            curUIState = IN_SETTINGS;
                            break;
                        }
                    }
                }
            }

            if (curSelectedElem < TEMPERATURE) {
                curSelectedElem = SETTINGS;
            } else if (curSelectedElem > SETTINGS) {
                curSelectedElem = TEMPERATURE;
            }

            for (int i = TEMPERATURE; i <= SETTINGS; i++) {
                drawElement(i, (i * ELEMENT_UI_SIZE) + 2, ELEMENTS_Y_OFFSET + 2);
            }

            oled.drawXbm(ELEMENT_UI_SIZE * curSelectedElem, ELEMENTS_Y_OFFSET, ELEMENT_UI_SIZE, ELEMENT_UI_SIZE, Selection_Icon);

            String tempHumid = String("Room: ") + roomTemp + String("C ") + humidity + String("%");
            drawCenteredUI(UIStrings[curSelectedElem * 2], UIStrings[(curSelectedElem * 2) + 1], tempHumid.c_str());

            break;
        }

        case IN_SETTINGS: {
            if ((left_btn + right_btn + a_btn + b_btn) == 1) { // only one button or else ignore
                if (left_btn) {
                    curSelectedElem--;
                } else if (right_btn) {
                    curSelectedElem++;
                } else {
                    switch (curSelectedElem) {
                        case LG_AKB7REM: {
                            curRemote = LG_AKB7;
                            break;
                        }
                        case MITSUBREM: {
                            curRemote = MITSUB;
                            break;
                        }
                        case DAIKINREM: {
                            curRemote = DAIKIN;
                            break;
                        }
                        case EXIT: {
                            curUIState = IN_MAIN;
                            break;
                        }
                    }
                }
            }

            if (curSelectedElem < LG_AKB7REM) {
                curSelectedElem = EXIT;
            } else if (curSelectedElem > EXIT) {
                curSelectedElem = LG_AKB7REM;
            }

            for (int i = LG_AKB7REM; i <= EXIT; i++) {
                int j = i - 4;
                drawElement(i, (j * ELEMENT_UI_SIZE) + 2, ELEMENTS_Y_OFFSET + 2);
            }

            oled.drawXbm(ELEMENT_UI_SIZE * (curSelectedElem - 4), ELEMENTS_Y_OFFSET, ELEMENT_UI_SIZE, ELEMENT_UI_SIZE, Selection_Icon);

            String remName = String("Current: ") + UIStrings[(curRemote * 2) + 8];
            drawCenteredUI(UIStrings[curSelectedElem * 2], UIStrings[(curSelectedElem * 2) + 1], remName.c_str());

            break;
        }
    }

    oled.display();
}

void disp_setTempPower(int temp, bool power) {
    curState.temp = temp;
    curState.power = power;
}

Remote disp_getCurRemote() {
    return curRemote;
}

ACState disp_getCurState() {
    return curState;
}