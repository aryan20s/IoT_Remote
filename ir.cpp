#include <Arduino.h>

#define DECODE_DISTANCE_WIDTH
#define RAW_BUFFER_LENGTH 750
#define NO_LED_FEEDBACK_CODE
#define RECORD_GAP_MICROS 50000

#include <IRremote.hpp>

#include "pinconstants.h"
#include "pins.h"
#include "ir.h"
#include "utils.h"

const char* REMOTE_TYPES[] = {
    "LG_AKB7", 
    "MITSUBISHI", 
    "DAIKIN", 
    "UNIMPL3",
};

#define SEND_BUTTON_PIN                     APPLICATION_PIN
#define DELAY_BETWEEN_REPEATS_MILLIS        70

void IR_sendLG(uint8_t temperature, uint8_t fanSpeed, bool power) {
    temperature = clamp(temperature, 18, 30);

    uint32_t dat = 0;
    if (!power) { 
        dat = 0x8A00311; 
    } else {
        uint32_t a = (temperature - 15) << 8;
        uint32_t b = 0;
        if (fanSpeed == 0) {
            b = 0x50;
            b |= (temperature + 6) & 0xF;
        }
        else if (fanSpeed == 1) {
            b = 0;
            b |= (temperature + 2) & 0xF;
        }
        else if (fanSpeed == 2) {
            b = 0x20;
            b |= (temperature + 3) & 0xF;
        }
        else if (fanSpeed == 3) {
            b = 0x40;
            b |= (temperature + 5) & 0xF;
        }

        dat = reverse32bit(0x8800000 | a | b) >> 4;
    }

    // pretend that all packets switch on the AC
    // if (!just_switched_on) {
    //     dat ^= 0b0000000000001000000000001000;
    // }
    
    IRRawDataType dataToSend[DECODED_RAW_DATA_ARRAY_SIZE] = { dat };
    DistanceWidthTimingInfoStruct timingInfo = { 3150, 9900, 500, 1600, 500, 550 };
    uint8_t noOfBits = 28;

    Serial.print("Sending ");
    Serial.print(REMOTE_TYPES[LG_AKB7]);
    Serial.print(" data: 0x");
    Serial.println(dat, HEX);

    IrSender.sendPulseDistanceWidthFromArray(
        38, &timingInfo, &dataToSend[0], 
        noOfBits, PROTOCOL_IS_LSB_FIRST, 100, 0
    );
}

void IR_sendMitsubishi(uint8_t temperature, uint8_t fanSpeed, bool power) {
    temperature = clamp(temperature, 19, 30);
    uint64_t dat;

    switch (fanSpeed) {
        case 0:
            dat = 0x1010;
        break;
        case 1:
            dat = 0x3010;
        break;
        case 2:
            dat = 0x5010;
        break;
        case 3:
            dat = 0x7010;
        break;
    }

    dat |= (temperature - 16) << 8; 

    if (power) {
        dat |= 0x0004;
    }
    
    IRRawDataType dataToSend[DECODED_RAW_DATA_ARRAY_SIZE] = { 0x300000002126CB23, 0xFFFBC0000F000004, 0xFF };
    dataToSend[0] |= dat << 44;
    dat = (((uint16_t) 0xFFFF) - ((uint16_t) dat)); // 16-bit inverse
    dataToSend[1] |= dat << 28;

    DistanceWidthTimingInfoStruct timingInfo = { 3200, 1600, 350, 1300, 350, 450 };
    uint8_t noOfBits = 136;

    Serial.print("Sending ");
    Serial.print(REMOTE_TYPES[MITSUB]);
    Serial.print(" data: 0x");
    Serial.print(dataToSend[0], HEX);
    Serial.print(" 0x");
    Serial.print(dataToSend[1], HEX);
    Serial.print(" 0x");
    Serial.println(dataToSend[2], HEX);

    IrSender.sendPulseDistanceWidthFromArray(
        38, &timingInfo, &dataToSend[0], 
        noOfBits, PROTOCOL_IS_LSB_FIRST, 100, 0
    );
}


void IR_sendDaikin(uint8_t temperature, uint8_t fanSpeed, bool power) {
    temperature = clamp(temperature, 18, 30);
    
    uint8_t frame1[7] = {0x11, 0xDA, 0x17, 0x48, 0x04, 0x00, 0x4E};
    uint8_t frame2[18] = {0x11, 0xDA, 0x17, 0x48, 0x00, 0x73, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00};
    
    if (power) {
        frame2[7] = 0x21;
    } else {
        frame2[7] = 0x20;
    }
    
    frame2[10] = (temperature * 2) - 18;
    
    if (fanSpeed == 0) frame2[11] = 0x06; 
    else if (fanSpeed == 1) frame2[11] = 0x16;
    else if (fanSpeed == 2) frame2[11] = 0x36;
    else if (fanSpeed >= 3) frame2[11] = 0x56;
    
    uint16_t checksum = 0;
    for (int i = 0; i < 17; i++) {
        checksum += frame2[i];
    }
    frame2[17] = checksum & 0xFF;
    
    auto sendFrame = [](uint8_t* payload, uint8_t lengthBytes) {
        uint64_t dataToSend[3] = {0, 0, 0}; 
        for (int i = 0; i < lengthBytes; i++) {
            dataToSend[i / 8] |= ((uint64_t)payload[i]) << ((i % 8) * 8);
        }
        DistanceWidthTimingInfoStruct timingInfo = { 5050, 2200, 350, 1850, 350, 750 };
        IrSender.sendPulseDistanceWidthFromArray(
            38, &timingInfo, &dataToSend[0], lengthBytes * 8, PROTOCOL_IS_LSB_FIRST, 100, 0
        );
    };
    
    Serial.print("Sending DAIKIN data (Power: ");
    Serial.print(power ? "ON" : "OFF");
    Serial.print(", Temp: ");
    Serial.print(temperature);
    Serial.print(", Fan: ");
    Serial.print(fanSpeed);
    Serial.println(")");
    
    sendFrame(frame1, 7);
    delay(13);
    sendFrame(frame2, 18);
}

void IR_init() {
    IrSender.begin(IR_SEND_PIN);
}

void IR_send(uint8_t temperature, uint8_t fanSpeed, bool power, Remote type) {
    switch (type) {
        case LG_AKB7:
            IR_sendLG(temperature, fanSpeed, power);
        break;
        case MITSUB:
            IR_sendMitsubishi(temperature, fanSpeed, power);
        break;
        case DAIKIN:
            IR_sendDaikin(temperature, fanSpeed, power);
        break;
        default:
            Serial.print("Unimplemented remote type: ");
            Serial.println(REMOTE_TYPES[type]);
        break;
    }
}

void IR_recv_loop() {
    IrReceiver.begin(IR_RECEIVE_PIN, false);
    pinMode(IR_RECEIVE_PIN, INPUT_PULLUP);
    IrReceiver.start();
    
    Serial.print(F("IR Learning Mode - waiting for signals on GPIO"));
    Serial.print(IR_RECEIVE_PIN);
    Serial.println(F("..."));

    while (true) {
        if (IrReceiver.decode()) {
            Serial.println(F("\n=== IR Signal Received ==="));
            IrReceiver.printIRResultShort(&Serial);
            
            Serial.print(F("Binary: "));
            int totalBits = IrReceiver.decodedIRData.numberOfBits;
            if (totalBits > 0) {
                int bitsPrinted = 0;
                for (int i = 0; i < DECODED_RAW_DATA_ARRAY_SIZE; i++) {
                    uint64_t chunk = IrReceiver.decodedIRData.decodedRawDataArray[i];
                    int bitsInChunk = (totalBits > 64) ? 64 : totalBits;
                    
                    for (int b = 0; b < bitsInChunk; b++) {
                        if (bitsPrinted > 0 && (bitsPrinted % 4 == 0)) {
                            Serial.print(" ");
                        }
                        Serial.print((uint8_t)((chunk >> b) & 1));
                        bitsPrinted++;
                    }
                    totalBits -= bitsInChunk;
                    if (totalBits <= 0) break;
                }
            } else {
                Serial.print(F("Unknown protocol / no bits"));
            }
            Serial.println();

            IrReceiver.printIRSendUsage(&Serial);
            IrReceiver.printIRResultRawFormatted(&Serial, true);
            Serial.println(F("==========================\n"));
            IrReceiver.resume();
        }
        delay(100);
    }
}
void IR_raw_loop() {
    Serial.print(F("Raw IR Mode - watching GPIO "));
    Serial.println(IR_RECEIVE_PIN);
    Serial.println(F("Waiting for IR signals... (Gap timeout: 50ms)"));
    Serial.println(F("Press 'A' button to toggle Compact/Full output."));
    
    IrReceiver.begin(IR_RECEIVE_PIN, false);
    pinMode(IR_RECEIVE_PIN, INPUT_PULLUP);
    IrReceiver.start();
    
    bool compactMode = false;
    bool prev_a = digitalRead(INP_A_PIN) == HIGH;
    
    while(true) {
        bool cur_a = digitalRead(INP_A_PIN) == HIGH;
        if (cur_a && !prev_a) {
            compactMode = !compactMode;
            Serial.print(F("\n*** Switched to "));
            Serial.print(compactMode ? F("COMPACT") : F("FULL"));
            Serial.println(F(" Logging Mode ***"));
        }
        prev_a = cur_a;
        
        if (IrReceiver.decode()) {
            
            int bitCount = 0;
            uint8_t currentByte = 0;
            int frameNum = 1;
            bool inFrame = false;
            String decodeOut = "";
            
            for (int i = 1; i < IrReceiver.decodedIRData.rawlen; i += 2) {
                uint32_t mark = IrReceiver.irparams.rawbuf[i] * MICROS_PER_TICK;
                
                if (i + 1 >= IrReceiver.decodedIRData.rawlen) {
                    break; 
                }
                uint32_t space = IrReceiver.irparams.rawbuf[i + 1] * MICROS_PER_TICK;
                
                if (mark > 2000) {
                    if (inFrame) {
                        if (bitCount > 0) {
                            char buf[32];
                            sprintf(buf, " (Partial: %02X)", currentByte);
                            decodeOut += buf;
                        }
                        decodeOut += "\n";
                    }
                    decodeOut += "Frame ";
                    decodeOut += String(frameNum++);
                    decodeOut += ": ";
                    bitCount = 0;
                    currentByte = 0;
                    inFrame = true;
                    continue; 
                }
                
                if (inFrame) {
                    if (space > 1200 && space < 3000) {
                        currentByte |= (1 << bitCount);
                    } else if (space > 300 && space <= 1200) {
                        // 0 bit
                    } else if (space > 5000) {
                        if (bitCount > 0) {
                            char buf[32];
                            sprintf(buf, " (Partial: %02X)", currentByte);
                            decodeOut += buf;
                        }
                        decodeOut += "\n";
                        inFrame = false;
                        continue;
                    }
                    
                    if (space <= 3000) {
                        bitCount++;
                        if (bitCount == 8) {
                            char buf[16];
                            sprintf(buf, "%02X ", currentByte);
                            decodeOut += buf;
                            currentByte = 0;
                            bitCount = 0;
                        }
                    }
                }
            }
            
            if (inFrame && bitCount > 0) {
                char buf[32];
                sprintf(buf, " (Partial: %02X)", currentByte);
                decodeOut += buf;
                decodeOut += "\n";
            }
            
            if (compactMode) {
                if (decodeOut.length() > 0) {
                    Serial.println(F("\n--- AC Signal (Compact) ---"));
                    Serial.print(decodeOut);
                }
            } else {
                Serial.println(F("\n--- IR Signal (Full) ---"));
                IrReceiver.printIRResultShort(&Serial);
                
                if (IrReceiver.decodedIRData.numberOfBits > 0) {
                    Serial.print(F("Raw Data (Hex): "));
                    for (int i = 0; i < DECODED_RAW_DATA_ARRAY_SIZE; i++) {
                        uint64_t chunk = IrReceiver.decodedIRData.decodedRawDataArray[i];
                        if (chunk == 0 && i > 0) break;
                        Serial.print("0x");
                        Serial.print((unsigned long)(chunk >> 32), HEX);
                        Serial.print((unsigned long)(chunk & 0xFFFFFFFF), HEX);
                        Serial.print(" ");
                    }
                    Serial.println();
                }
                
                if (decodeOut.length() > 0) {
                    Serial.println(F("AC Protocol Decode (Hex Bytes):"));
                    Serial.print(decodeOut);
                }
                
                IrReceiver.printIRSendUsage(&Serial);
                IrReceiver.printIRResultRawFormatted(&Serial, true);
                Serial.println(F("-----------------"));
            }
            
            IrReceiver.resume();
        }
        delay(10);
    }
}
