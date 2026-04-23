#include "DataManager.h"
#include <algorithm>
#include <SD.h>

namespace {
const char* getPokemonIndexPath(AppLanguage language) {
  return (language == APP_LANGUAGE_EN) ? "/pokemon/index_en.json" : "/pokemon/index.json";
}
}

DataManager::DataManager() {
  currentPokemon = {};
  pokemonNames.resize(MIN_POKEMON_ID + 1);
  maxPokemonId = MIN_POKEMON_ID;
  currentPokemonId = MIN_POKEMON_ID;
  currentLanguage = APP_LANGUAGE_JA;
}

bool DataManager::begin() {
  return SD.exists("/pokemon") && loadPokemonIndex();
}

bool DataManager::setLanguage(AppLanguage language) {
  if (language < APP_LANGUAGE_JA || language >= APP_LANGUAGE_COUNT) {
    language = APP_LANGUAGE_JA;
  }
  if (currentLanguage == language && !pokemonNames.empty()) {
    return true;
  }
  const AppLanguage previousLanguage = currentLanguage;
  currentLanguage = language;
  if (!loadPokemonIndex()) {
    currentLanguage = previousLanguage;
    return false;
  }
  if (currentPokemonId >= MIN_POKEMON_ID) {
    return loadPokemonDetail(currentPokemonId);
  }
  return true;
}

bool DataManager::loadPokemonIndex() {
  File file = SD.open(getPokemonIndexPath(currentLanguage));
  if (!file) return false;

  JsonDocument doc;
#ifdef POKEMONCOLLECTION_SIM
  DeserializationError error = deserializeJson(doc, file.contents().c_str());
  file.close();
#else
  DeserializationError error = deserializeJson(doc, file);
  file.close();
#endif
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
    pokemonNames[id] = String(entry["n"] | "");
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
  currentPokemonId = id;
  char filename[64];
  snprintf(filename, sizeof(filename), "/pokemon/details/%04d.json", id);
  File file = SD.open(filename);
  if (!file) return false;

  JsonDocument doc;
#ifdef POKEMONCOLLECTION_SIM
  DeserializationError error = deserializeJson(doc, file.contents().c_str());
#else
  DeserializationError error = deserializeJson(doc, file);
#endif
  if (error) return false;

  currentPokemon.id = doc["id"];
  currentPokemon.name = getPokemonName(id);
  if (currentPokemon.name.length() == 0) {
    currentPokemon.name = String(doc["name"] | "");
  }
  currentPokemon.category = String(doc["category"] | "");
  currentPokemon.height = String(doc["height"] | "");
  currentPokemon.weight = String(doc["weight"] | "");
  currentPokemon.description = String(doc["desc"] | "");

  currentPokemon.types.clear();
  for (const char* t : doc["types"].as<JsonArray>()) currentPokemon.types.push_back(String(t != nullptr ? t : ""));

  currentPokemon.abilities.clear();
  for (JsonObject ab : doc["abilities"].as<JsonArray>()) {
    currentPokemon.abilities.push_back({String(ab["n"] | ""), String(ab["d"] | "")});
  }

  currentPokemon.evolutions.clear();
  for (JsonObject evo : doc["evolutions"].as<JsonArray>()) {
    const uint16_t evolutionId = static_cast<uint16_t>(evo["id"] | 0);
    String evolutionName = getPokemonName(evolutionId);
    if (evolutionName.length() == 0) {
      evolutionName = String(evo["n"] | "");
    }
    currentPokemon.evolutions.push_back({evolutionId, evolutionName});
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

  String normalizedQuery = query;
  normalizedQuery.toLowerCase();

  size_t skipped = 0;
  for (uint16_t id = MIN_POKEMON_ID; id <= maxPokemonId; ++id) {
    const String& name = pokemonNames[id];
    if (name.length() == 0) {
      continue;
    }
    if (normalizedQuery.length() > 0) {
      String normalizedName = name;
      normalizedName.toLowerCase();
      if (normalizedName.indexOf(normalizedQuery) < 0) {
        continue;
      }
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
