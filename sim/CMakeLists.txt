cmake_minimum_required(VERSION 3.16)
project(PokemonCollectionSimulator LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures" FORCE)
endif()

set(SDL2_PREFIX_HINTS
  /opt/homebrew/opt/sdl2
  /opt/homebrew/opt/sdl2_image
  /opt/homebrew/opt/sdl2_ttf
)

set(ARDUINOJSON_ROOT "" CACHE PATH "Path to an ArduinoJson checkout or install root")
option(POKEMONCOLLECTION_SIM_FETCH_ARDUINOJSON "Fetch ArduinoJson automatically when it is not installed locally" ON)

set(ARDUINOJSON_SEARCH_PATHS)
if(ARDUINOJSON_ROOT)
  list(APPEND ARDUINOJSON_SEARCH_PATHS
    "${ARDUINOJSON_ROOT}"
    "${ARDUINOJSON_ROOT}/src"
  )
endif()

file(GLOB ARDUINOJSON_PIO_CANDIDATES
  "${CMAKE_CURRENT_SOURCE_DIR}/../.pio/libdeps/*/ArduinoJson/src"
)
list(APPEND ARDUINOJSON_SEARCH_PATHS
  "${CMAKE_CURRENT_SOURCE_DIR}/local-deps/ArduinoJson"
  "${CMAKE_CURRENT_SOURCE_DIR}/local-deps/ArduinoJson/src"
  ${ARDUINOJSON_PIO_CANDIDATES}
  "${CMAKE_CURRENT_SOURCE_DIR}/../.pio/libdeps/m5stack-cores3/ArduinoJson/src"
  "$ENV{HOME}/.platformio/lib/ArduinoJson/src"
)

find_path(ARDUINOJSON_INCLUDE_DIR ArduinoJson.h
  HINTS ${ARDUINOJSON_SEARCH_PATHS}
  PATH_SUFFIXES src
)

if(NOT ARDUINOJSON_INCLUDE_DIR)
  if(POKEMONCOLLECTION_SIM_FETCH_ARDUINOJSON)
    message(STATUS "ArduinoJson.h was not found locally. Fetching ArduinoJson for sim build.")
    FetchContent_Declare(
      arduinojson
      GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
      GIT_TAG v7.4.2
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(arduinojson)
    set(ARDUINOJSON_INCLUDE_DIR "${arduinojson_SOURCE_DIR}/src")
  else()
    message(FATAL_ERROR
      "ArduinoJson.h was not found. Install PlatformIO dependencies with "
      "'pio run -e m5stack-cores3', pass -DARDUINOJSON_ROOT=/path/to/ArduinoJson, "
      "or enable -DPOKEMONCOLLECTION_SIM_FETCH_ARDUINOJSON=ON."
    )
  endif()
endif()

find_package(SDL2 REQUIRED CONFIG HINTS ${SDL2_PREFIX_HINTS})
find_package(SDL2_ttf QUIET CONFIG HINTS ${SDL2_PREFIX_HINTS})
find_package(SDL2_image QUIET CONFIG HINTS ${SDL2_PREFIX_HINTS})

if(NOT TARGET SDL2_ttf::SDL2_ttf)
  find_path(SDL2_TTF_INCLUDE_DIR SDL_ttf.h
    HINTS /opt/homebrew/opt/sdl2_ttf/include/SDL2
  )
  find_library(SDL2_TTF_LIBRARY SDL2_ttf
    HINTS /opt/homebrew/opt/sdl2_ttf/lib
  )
  if(NOT SDL2_TTF_INCLUDE_DIR OR NOT SDL2_TTF_LIBRARY)
    message(FATAL_ERROR
      "SDL2_ttf was not found. Install it first. On macOS/Homebrew, run "
      "'brew install sdl2_ttf'."
    )
  endif()
endif()

if(NOT TARGET SDL2_image::SDL2_image)
  find_path(SDL2_IMAGE_INCLUDE_DIR SDL_image.h
    HINTS /opt/homebrew/opt/sdl2_image/include/SDL2
  )
  find_library(SDL2_IMAGE_LIBRARY SDL2_image
    HINTS /opt/homebrew/opt/sdl2_image/lib
  )
  if(NOT SDL2_IMAGE_INCLUDE_DIR OR NOT SDL2_IMAGE_LIBRARY)
    message(FATAL_ERROR
      "SDL2_image was not found. Install it first. On macOS/Homebrew, run "
      "'brew install sdl2_image'."
    )
  endif()
endif()

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
  ${ARDUINOJSON_INCLUDE_DIR}
  include
  ../include
)

if(SDL2_TTF_INCLUDE_DIR)
  target_include_directories(pokemoncollection_sim PRIVATE ${SDL2_TTF_INCLUDE_DIR})
endif()

if(SDL2_IMAGE_INCLUDE_DIR)
  target_include_directories(pokemoncollection_sim PRIVATE ${SDL2_IMAGE_INCLUDE_DIR})
endif()

target_link_libraries(pokemoncollection_sim PRIVATE
  SDL2::SDL2
)

if(TARGET SDL2_ttf::SDL2_ttf)
  target_link_libraries(pokemoncollection_sim PRIVATE SDL2_ttf::SDL2_ttf)
else()
  target_link_libraries(pokemoncollection_sim PRIVATE ${SDL2_TTF_LIBRARY})
endif()

if(TARGET SDL2_image::SDL2_image)
  target_link_libraries(pokemoncollection_sim PRIVATE SDL2_image::SDL2_image)
else()
  target_link_libraries(pokemoncollection_sim PRIVATE ${SDL2_IMAGE_LIBRARY})
endif()
