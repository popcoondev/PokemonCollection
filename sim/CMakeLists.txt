cmake_minimum_required(VERSION 3.16)
project(PokemonCollectionSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures" FORCE)
endif()

find_package(SDL2 REQUIRED)

add_executable(pokemoncollection_sim
  main.cpp
)

target_include_directories(pokemoncollection_sim PRIVATE
  ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(pokemoncollection_sim PRIVATE
  ${SDL2_LIBRARIES}
)
