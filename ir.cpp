#include <Arduino.h>
#include <IRremote.hpp>

#include "pinconstants.h"
#include "ir.h"
#include "utils.h"

const char* REMOTE_TYPES[] = {
    "LG_AKB7", 
    "MITSUBISHI", 
    "UNIMPL2", 
    "UNIMPL3",
};

#define DECODE_DISTANCE_WIDTH
#ifndef RAW_BUFFER_LENGTH
  #define RAW_BUFFER_LENGTH 750
#endif

#define NO_LED_FEEDBACK_CODE
#define RECORD_GAP_MICROS 12000

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
    
    IRRawDataType dataToSend[RAW_DATA_ARRAY_SIZE] = { dat };
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
    
    IRRawDataType dataToSend[RAW_DATA_ARRAY_SIZE] = { 0x300000002126CB23, 0xFFFBC0000F000004, 0xFF };
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
        default:
            Serial.print("Unimplemented remote type: ");
            Serial.println(REMOTE_TYPES[type]);
        break;
    }
}

IRRawDataType sDecodedRawDataArray[RAW_DATA_ARRAY_SIZE] = { 0x7B34ED12 }; // Initialize with NEC address 0x12 and command 0x34
DistanceWidthTimingInfoStruct sDistanceWidthTimingInfo = { 9000, 4500, 560, 1690, 560, 560 }; // Initialize with NEC timing
uint8_t sNumberOfBits = 32;
void IR_recv_loop() {
    IrReceiver.begin(IR_RECEIVE_PIN, false);
    IrReceiver.start();
    
    while (true) {
        if (IrReceiver.decode()) {
            IrReceiver.printIRResultShort(&Serial);
            if (IrReceiver.decodedIRData.protocol != UNKNOWN) {
                IrReceiver.printIRSendUsage(&Serial);

                if (memcmp(&sDistanceWidthTimingInfo, &IrReceiver.decodedIRData.DistanceWidthTimingInfo,
                        sizeof(sDistanceWidthTimingInfo)) != 0) {
                    Serial.print(F("Store new timing info data="));
                    IrReceiver.printDistanceWidthTimingInfo(&Serial, &IrReceiver.decodedIRData.DistanceWidthTimingInfo);
                    Serial.println();
                    sDistanceWidthTimingInfo = IrReceiver.decodedIRData.DistanceWidthTimingInfo; // copy content here
                }
            }
            IrReceiver.resume();
        }

        delay(100);
    }
}