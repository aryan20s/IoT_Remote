#define INP_L_PIN 16
#define INP_R_PIN 0
#define INP_A_PIN 15
#define INP_B_PIN 2

#define ELEMENT_UI_SIZE 32
#define ELEMENT_ICON_SIZE 28
#define ELEMENTS_Y_OFFSET 12

#include <Arduino.h>
#include <U8g2lib.h>

#include "ir.h"
#include "utils.h"
#include "display.h"

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, 5, 4, U8X8_PIN_NONE);

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
    UNIMPL2REM = 6,
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
    "UNIMPL_2",
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

    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.sendBuffer();
}

U8G2_SSD1306_128X64_NONAME_F_SW_I2C* disp_getu8g2() {
    return &u8g2;
}

void drawElement(int element, int x, int y) {
    u8g2.setDrawColor(1);
    u8g2.drawFrame(x++, y++, ELEMENT_ICON_SIZE, ELEMENT_ICON_SIZE);
    u8g2.drawFrame(x++, y++, ELEMENT_ICON_SIZE - 2, ELEMENT_ICON_SIZE - 2);

    switch (element) {
        case TEMPERATURE: {
            String disp = String(curState.temp);
            disp += 'C';
            u8g2.setFont(u8g2_font_6x12_tf);
            u8g2.setFontMode(1);
            u8g2.drawStr(x + 3, y + (ELEMENT_ICON_SIZE / 2) + 1, disp.c_str());
            break;
        }
        case FAN: {
            for (int i = 0; i < curState.fanSpd; i++) {
                u8g2.drawBox(x + (i * 8) + 1, y + 1, 6, 22);
            }
            break;
        }
        case POWER: {
            int xPos = curState.power ? x + 6: x + 3;
            u8g2.setFont(u8g2_font_6x12_tf);
            u8g2.setFontMode(1);
            u8g2.drawStr(xPos, y + (ELEMENT_ICON_SIZE / 2) + 1, curState.power ? "ON" : "OFF");
            break;
        }
        case SETTINGS: {
            u8g2.setBitmapMode(1);
            u8g2.drawXBMP(x, y, 24, 24, Settings_Icon);
            break;
        }
        case LG_AKB7REM: case MITSUBREM: case UNIMPL2REM: {
            u8g2.setBitmapMode(1);
            u8g2.drawXBMP(x, y, 24, 24, Remote_Icon);
            break;
        }
        case EXIT: {
            u8g2.setBitmapMode(1);
            u8g2.drawXBMP(x, y, 24, 24, Exit_Icon);
            break;
        }
    }
}

void disp_update(float roomTemp, float humidity) {
    u8g2.clearBuffer();

    bool left_btn  = !digitalRead(INP_L_PIN);
    bool right_btn = !digitalRead(INP_R_PIN);
    bool b_btn     = !digitalRead(INP_B_PIN);
    bool a_btn     =  digitalRead(INP_A_PIN);

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

            u8g2.setBitmapMode(1);
            u8g2.drawXBMP(ELEMENT_UI_SIZE * curSelectedElem, ELEMENTS_Y_OFFSET, ELEMENT_UI_SIZE, ELEMENT_UI_SIZE, Selection_Icon);
            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.setFontMode(1);

            const char* str = UIStrings[curSelectedElem * 2];
            int xPos = 64 - (strlen(str) * 5) / 2; // centering
            u8g2.drawStr(xPos, ELEMENTS_Y_OFFSET + ELEMENT_UI_SIZE + 7 + 1, str);

            str = UIStrings[(curSelectedElem * 2) + 1];
            xPos = 64 - (strlen(str) * 5) / 2; // centering
            u8g2.drawStr(xPos, ELEMENTS_Y_OFFSET + ELEMENT_UI_SIZE + 7 + 8 + 1, str);
            
            String tempHumid = String("Room: ") + roomTemp + String("C ") + humidity + String("%");
            xPos = 64 - (tempHumid.length() * 5) / 2; // centering
            u8g2.drawStr(xPos, ELEMENTS_Y_OFFSET - 3, tempHumid.c_str());

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
                        case UNIMPL2REM: {
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

            u8g2.setBitmapMode(1);
            u8g2.drawXBMP(ELEMENT_UI_SIZE * (curSelectedElem - 4), ELEMENTS_Y_OFFSET, ELEMENT_UI_SIZE, ELEMENT_UI_SIZE, Selection_Icon);
            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.setFontMode(1);

            const char* str = UIStrings[curSelectedElem * 2];
            int xPos = 64 - (strlen(str) * 5) / 2; // centering
            u8g2.drawStr(xPos, ELEMENTS_Y_OFFSET + ELEMENT_UI_SIZE + 7 + 1, str);

            str = UIStrings[(curSelectedElem * 2) + 1];
            xPos = 64 - (strlen(str) * 5) / 2; // centering
            u8g2.drawStr(xPos, ELEMENTS_Y_OFFSET + ELEMENT_UI_SIZE + 7 + 8 + 1, str);

            String remName = String("Current: ") + UIStrings[(curRemote * 2) + 8];
            xPos = 64 - (remName.length() * 5) / 2; // centering
            u8g2.drawStr(xPos, ELEMENTS_Y_OFFSET - 3, remName.c_str());

            break;
        }
    }

    u8g2.sendBuffer();
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