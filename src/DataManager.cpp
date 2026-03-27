#include "DataManager.h"
#include <SD.h>

DataManager::DataManager() {
  currentPokemon = {};
}

bool DataManager::begin() {
  return SD.exists("/pokemon");
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
