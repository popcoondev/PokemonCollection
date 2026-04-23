#include "AppSensors.h"

#ifdef POKEMONCOLLECTION_SIM

uint8_t appI2CReadRegister8(uint8_t address, uint8_t reg, uint32_t freq) {
  (void)address;
  (void)reg;
  (void)freq;
  return 0xFF;
}

bool appI2CWriteRegister8(uint8_t address, uint8_t reg, uint8_t value, uint32_t freq) {
  (void)address;
  (void)reg;
  (void)value;
  (void)freq;
  return false;
}

bool appI2CReadRegister(uint8_t address, uint8_t reg, uint8_t* data, size_t len, uint32_t freq) {
  (void)address;
  (void)reg;
  (void)data;
  (void)len;
  (void)freq;
  return false;
}

#else

#include <M5Unified.h>

uint8_t appI2CReadRegister8(uint8_t address, uint8_t reg, uint32_t freq) {
  return M5.In_I2C.readRegister8(address, reg, freq);
}

bool appI2CWriteRegister8(uint8_t address, uint8_t reg, uint8_t value, uint32_t freq) {
  return M5.In_I2C.writeRegister8(address, reg, value, freq);
}

bool appI2CReadRegister(uint8_t address, uint8_t reg, uint8_t* data, size_t len, uint32_t freq) {
  return M5.In_I2C.readRegister(address, reg, data, len, freq);
}

#endif
