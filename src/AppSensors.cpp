#include "AppSensors.h"

#ifdef POKEMONCOLLECTION_SIM

#include "SimPlatform.h"

uint8_t appI2CReadRegister8(uint8_t address, uint8_t reg, uint32_t freq) {
  (void)address;
  (void)freq;
  if (reg == 0x86) {
    return 0x92;
  }
  return 0xFF;
}

bool appI2CWriteRegister8(uint8_t address, uint8_t reg, uint8_t value, uint32_t freq) {
  (void)address;
  (void)reg;
  (void)value;
  (void)freq;
  return true;
}

bool appI2CReadRegister(uint8_t address, uint8_t reg, uint8_t* data, size_t len, uint32_t freq) {
  (void)address;
  (void)freq;
  if (data == nullptr || len == 0) {
    return false;
  }
  const uint16_t value = SimPlatform::getProximityValue();
  if (reg == 0x8D && len >= 1) {
    data[0] = static_cast<uint8_t>(value & 0xFF);
    return true;
  }
  if (reg == 0x8E && len >= 1) {
    data[0] = static_cast<uint8_t>((value >> 8) & 0x07);
    return true;
  }
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
