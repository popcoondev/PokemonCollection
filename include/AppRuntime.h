#ifndef APP_RUNTIME_H
#define APP_RUNTIME_H

class AppRuntime {
public:
  void boot();
  void tick();
};

AppRuntime& appRuntimeInstance();
void appBoot();
void appTick();

#endif
