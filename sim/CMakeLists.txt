cmake_minimum_required(VERSION 3.16)
project(PokemonCollectionSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures" FORCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)

add_executable(pokemoncollection_sim
  main.cpp
  SimSd.cpp
  LgfxSpriteShim.cpp
  M5RendererShim.cpp
  ../src/PcRenderer.cpp
  ../src/DataManager.cpp
  ../src/ImageLoader.cpp
  ../src/UIController.cpp
)

target_include_directories(pokemoncollection_sim PRIVATE
  ../sim/include
  ../include
  ../.pio/libdeps/m5stack-cores3/ArduinoJson/src
)

target_link_libraries(pokemoncollection_sim PRIVATE
  PkgConfig::SDL2
  PkgConfig::SDL2_TTF
  PkgConfig::SDL2_IMAGE
)

target_compile_definitions(pokemoncollection_sim PRIVATE
  POKEMONCOLLECTION_SIM_NATIVE
)
