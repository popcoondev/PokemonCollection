#include "DataManager.h"
#include <SD.h>

DataManager::DataManager() {
  currentPokemon = {};
  pokemonNames.resize(MAX_POKEMON_ID + 1);
}

bool DataManager::begin() {
  return SD.exists("/pokemon") && loadPokemonIndex();
}

bool DataManager::loadPokemonIndex() {
  File file = SD.open("/pokemon/index.json");
  if (!file) return false;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error || !doc.is<JsonArray>()) return false;

  for (auto& name : pokemonNames) {
    name = "";
  }

  for (JsonObject entry : doc.as<JsonArray>()) {
    const uint16_t id = static_cast<uint16_t>(entry["id"] | 0);
    if (id >= pokemonNames.size()) continue;
    pokemonNames[id] = entry["n"].as<String>();
  }

  return true;
}

bool DataManager::loadPokemonDetail(uint16_t id) {
  char filename[64];
  snprintf(filename, sizeof(filename), "/pokemon/details/%04d.json", id);
  File file = SD.open(filename);
  if (!file) return false;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error) return false;

  currentPokemon.id = doc["id"];
  currentPokemon.name = doc["name"].as<String>();
  currentPokemon.category = doc["category"].as<String>();
  currentPokemon.height = doc["height"].as<String>();
  currentPokemon.weight = doc["weight"].as<String>();
  currentPokemon.description = doc["desc"].as<String>();

  currentPokemon.types.clear();
  for (const char* t : doc["types"].as<JsonArray>()) currentPokemon.types.push_back(String(t));

  currentPokemon.abilities.clear();
  for (JsonObject ab : doc["abilities"].as<JsonArray>()) {
    currentPokemon.abilities.push_back({ab["n"].as<String>(), ab["d"].as<String>()});
  }

  currentPokemon.evolutions.clear();
  for (JsonObject evo : doc["evolutions"].as<JsonArray>()) {
    currentPokemon.evolutions.push_back({static_cast<uint16_t>(evo["id"] | 0), evo["n"].as<String>()});
  }

  file.close();
  return true;
}

const String& DataManager::getPokemonName(uint16_t id) const {
  static const String emptyName = "";
  if (id >= pokemonNames.size()) return emptyName;
  return pokemonNames[id];
}
