#include "AppMotion.h"

#ifdef POKEMONCOLLECTION_SIM

bool appReadAccel(float* ax, float* ay, float* az) {
  if (ax) *ax = 0.0f;
  if (ay) *ay = 0.0f;
  if (az) *az = 0.0f;
  return false;
}

#else

#include <M5Unified.h>

bool appReadAccel(float* ax, float* ay, float* az) {
  return M5.Imu.getAccel(ax, ay, az);
}

#endif
