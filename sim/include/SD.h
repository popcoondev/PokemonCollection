#ifndef SIM_SD_H
#define SIM_SD_H

#include "FS.h"

class SimSDClass {
public:
  bool begin();
  bool exists(const char* path) const;
  File open(const char* path, int mode = FILE_READ) const;
  bool mkdir(const char* path) const;
  bool remove(const char* path) const;
};

extern SimSDClass SD;

#endif
