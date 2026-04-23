#ifndef APP_SENSORS_H
#define APP_SENSORS_H

#include <Arduino.h>

uint8_t appI2CReadRegister8(uint8_t address, uint8_t reg, uint32_t freq);
bool appI2CWriteRegister8(uint8_t address, uint8_t reg, uint8_t value, uint32_t freq);
bool appI2CReadRegister(uint8_t address, uint8_t reg, uint8_t* data, size_t len, uint32_t freq);

#endif
