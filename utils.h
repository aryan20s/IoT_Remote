#ifndef MYUTILS_H
#define MYUTILS_H

#include <Arduino.h>

void printData(uint32_t num, int bits);
int clamp(int value, int min, int max);
uint32_t reverse32bit(uint32_t x);
void resetPins();

#endif