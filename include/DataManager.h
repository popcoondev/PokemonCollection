#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>
#include "Config.h"

struct PokemonDetail {
  struct Ability {
    String name;
    String description;
  };
  struct EvolutionEntry {
    uint16_t id;
    String name;
  };

  uint16_t id;
  String name;
  std::vector<String> types;
  String category;
  String height;
  String weight;
  String description;
  std::vector<Ability> abilities;
  std::vector<EvolutionEntry> evolutions;
};

class DataManager {
public:
  DataManager();
  bool begin();
  bool loadPokemonDetail(uint16_t id);
  const PokemonDetail& getCurrentPokemon() const { return currentPokemon; }
  const String& getPokemonName(uint16_t id) const;

private:
  bool loadPokemonIndex();
  PokemonDetail currentPokemon;
  std::vector<String> pokemonNames;
};

#endif
