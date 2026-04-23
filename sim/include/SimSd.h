#ifndef SIM_SD_BACKEND_H
#define SIM_SD_BACKEND_H

#include <string>

namespace SimSd {

bool begin();
std::string rootPath();
std::string resolvePath(const char* path);

}

#endif
