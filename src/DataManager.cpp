#include "DataManager.h"
#include <algorithm>
#include <SD.h>

DataManager::DataManager() {
  currentPokemon = {};
  pokemonNames.resize(MIN_POKEMON_ID + 1);
  maxPokemonId = MIN_POKEMON_ID;
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

  uint16_t detectedMaxId = MIN_POKEMON_ID;
  for (JsonObject entry : doc.as<JsonArray>()) {
    const uint16_t id = static_cast<uint16_t>(entry["id"] | 0);
    if (id > detectedMaxId) {
      detectedMaxId = id;
    }
  }

  maxPokemonId = detectedMaxId;
  pokemonNames.clear();
  pokemonNames.resize(maxPokemonId + 1);

  for (auto& name : pokemonNames) {
    name = "";
  }

  for (JsonObject entry : doc.as<JsonArray>()) {
    const uint16_t id = static_cast<uint16_t>(entry["id"] | 0);
    if (id >= pokemonNames.size()) continue;
    pokemonNames[id] = entry["n"].as<String>();
  }

  pokemonIdsSortedByName.clear();
  for (uint16_t id = MIN_POKEMON_ID; id <= maxPokemonId; ++id) {
    if (pokemonNames[id].length() > 0) {
      pokemonIdsSortedByName.push_back(id);
    }
  }
  std::sort(
      pokemonIdsSortedByName.begin(),
      pokemonIdsSortedByName.end(),
      [this](uint16_t lhs, uint16_t rhs) {
        const int cmp = pokemonNames[lhs].compareTo(pokemonNames[rhs]);
        return (cmp == 0) ? (lhs < rhs) : (cmp < 0);
      });

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

std::vector<uint16_t> DataManager::findPokemonIdsByName(const String& query, size_t offset, size_t limit) const {
  std::vector<uint16_t> results;
  if (limit == 0) {
    return results;
  }

  size_t skipped = 0;
  for (uint16_t id : pokemonIdsSortedByName) {
    const String& name = pokemonNames[id];
    if (query.length() > 0 && name.indexOf(query) < 0) {
      continue;
    }
    if (skipped < offset) {
      ++skipped;
      continue;
    }
    results.push_back(id);
    if (results.size() >= limit) {
      break;
    }
  }

  return results;
}
