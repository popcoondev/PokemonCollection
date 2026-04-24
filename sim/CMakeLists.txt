cmake_minimum_required(VERSION 3.16)
project(PokemonCollectionSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures" FORCE)
endif()

find_package(SDL2 REQUIRED)
find_path(SDL2_TTF_INCLUDE_DIR SDL_ttf.h
  HINTS /opt/homebrew/opt/sdl2_ttf/include/SDL2
)
find_library(SDL2_TTF_LIBRARY SDL2_ttf
  HINTS /opt/homebrew/opt/sdl2_ttf/lib
)

add_executable(pokemoncollection_sim
  main.cpp
  SimPlatform.cpp
  SimSd.cpp
  ../src/AppAudio.cpp
  ../src/AppConcurrency.cpp
  ../src/AppDevice.cpp
  ../src/AppInput.cpp
  ../src/AppMotion.cpp
  ../src/AppPlatform.cpp
  ../src/AppPower.cpp
  ../src/AppSensors.cpp
  ../src/AppStorage.cpp
  ../src/DataManager.cpp
  ../src/ImageLoader.cpp
  ../src/PcRenderer.cpp
  ../src/Renderer.cpp
  ../src/UIController.cpp
  ../src/main.cpp
)

target_compile_definitions(pokemoncollection_sim PRIVATE
  POKEMONCOLLECTION_SIM
)

target_include_directories(pokemoncollection_sim PRIVATE
  ${SDL2_INCLUDE_DIRS}
  ${SDL2_TTF_INCLUDE_DIR}
  include
  ../include
  ../.pio/libdeps/m5stack-cores3/ArduinoJson/src
)

target_link_libraries(pokemoncollection_sim PRIVATE
  ${SDL2_LIBRARIES}
  ${SDL2_TTF_LIBRARY}
)
