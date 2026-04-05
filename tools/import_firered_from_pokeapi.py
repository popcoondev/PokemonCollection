#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import time
import urllib.error
import urllib.request
from collections import defaultdict
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = ROOT / "data_src" / "firered_raw"
API_BASE = "https://pokeapi.co/api/v2"
USER_AGENT = "PokemonCollection/1.0 (+local data import)"


class ApiClient:
    def __init__(self, throttle_sec: float = 0.65) -> None:
        self.cache: dict[str, Any] = {}
        self.throttle_sec = throttle_sec

    def get_json(self, url: str) -> Any:
        if url in self.cache:
            return self.cache[url]
        req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                payload = json.load(resp)
        except urllib.error.URLError as exc:
            raise RuntimeError(f"failed to fetch {url}: {exc}") from exc
        self.cache[url] = payload
        time.sleep(self.throttle_sec)
        return payload

    def api(self, path: str) -> Any:
        return self.get_json(f"{API_BASE}{path}")


def slug_to_ja_fallback(slug: str) -> str:
    text = slug.replace("kanto-", "").replace("-area", "").replace("-", " ")
    return text


def pick_name(entries: list[dict[str, Any]], preferred_langs: tuple[str, ...]) -> str | None:
    for lang in preferred_langs:
        for entry in entries:
            language = entry.get("language", {}).get("name")
            if language == lang:
                return entry.get("name")
    return None


def extract_location_name(client: ApiClient, location_area_url: str) -> tuple[str, str]:
    area = client.get_json(location_area_url)
    area_name = pick_name(area.get("names", []), ("ja-Hrkt", "ja", "en"))
    if area_name:
        return area["name"], area_name

    location_url = area.get("location", {}).get("url")
    if location_url:
        location = client.get_json(location_url)
        location_name = pick_name(location.get("names", []), ("ja-Hrkt", "ja", "en"))
        if location_name:
            return location["name"], location_name

    slug = area.get("name", "")
    return slug, slug_to_ja_fallback(slug)


def normalize_method(method: str) -> str:
    mapping = {
        "walk": "くさむら",
        "surf": "なみのり",
        "old-rod": "ボロのつりざお",
        "good-rod": "いいつりざお",
        "super-rod": "すごいつりざお",
        "gift": "もらう",
    }
    return mapping.get(method, method)


def fetch_encounters(client: ApiClient, pokemon_id: int) -> tuple[list[dict[str, Any]], dict[str, dict[str, Any]]]:
    encounters = client.api(f"/pokemon/{pokemon_id}/encounters")
    rows: list[dict[str, Any]] = []
    location_master: dict[str, dict[str, Any]] = {}

    for area_entry in encounters:
        area_slug, area_name = extract_location_name(client, area_entry["location_area"]["url"])
        location_id = area_slug.replace("-", "_")
        location_master.setdefault(
            location_id,
            {
                "location_id": location_id,
                "name": area_name,
                "order": 9999,
                "postgame_only": "false",
            },
        )

        for version_detail in area_entry.get("version_details", []):
            if version_detail.get("version", {}).get("name") != "firered":
                continue
            for encounter in version_detail.get("encounter_details", []):
                rows.append(
                    {
                        "location_id": location_id,
                        "pokemon_id": str(pokemon_id),
                        "method": normalize_method(encounter.get("method", {}).get("name", "")),
                        "rate": f"{encounter.get('chance', 0)}%",
                        "min_level": str(encounter.get("min_level", 0)),
                        "max_level": str(encounter.get("max_level", 0)),
                        "postgame_only": "false",
                    }
                )

    return rows, location_master


def fetch_level_up_and_machines(client: ApiClient, pokemon_id: int) -> tuple[list[dict[str, str]], list[dict[str, str]]]:
    pokemon = client.api(f"/pokemon/{pokemon_id}")
    level_rows: list[dict[str, str]] = []
    machine_rows: list[dict[str, str]] = []

    level_sort = 10
    machine_sort = 10

    for move_entry in pokemon.get("moves", []):
        move_name = move_entry.get("move", {}).get("name", "").replace("-", " ")
        for detail in move_entry.get("version_group_details", []):
            if detail.get("version_group", {}).get("name") != "firered-leafgreen":
                continue
            method = detail.get("move_learn_method", {}).get("name")
            if method == "level-up":
                level_rows.append(
                    {
                        "pokemon_id": str(pokemon_id),
                        "sort_order": str(level_sort),
                        "level": str(detail.get("level_learned_at", 0)),
                        "move": move_name,
                    }
                )
                level_sort += 10
            elif method == "machine":
                machine_rows.append(
                    {
                        "pokemon_id": str(pokemon_id),
                        "sort_order": str(machine_sort),
                        "kind": "machine",
                        "move": move_name,
                    }
                )
                machine_sort += 10

    level_rows.sort(key=lambda row: (int(row["level"]), int(row["sort_order"]), row["move"]))
    machine_rows.sort(key=lambda row: row["move"])
    return level_rows, machine_rows


def flatten_chain(chain: dict[str, Any], rows: list[tuple[str, list[dict[str, Any]], list[Any]]]) -> None:
    rows.append((chain["species"]["name"], chain.get("evolution_details", []), chain.get("evolves_to", [])))
    for child in chain.get("evolves_to", []):
        flatten_chain(child, rows)


def describe_evolution(detail: dict[str, Any]) -> str:
    min_level = detail.get("min_level")
    item_data = detail.get("item") or {}
    trigger_data = detail.get("trigger") or {}
    held_item_data = detail.get("held_item") or {}
    trade_species_data = detail.get("trade_species") or {}
    item = item_data.get("name")
    trigger = trigger_data.get("name")
    held_item = held_item_data.get("name")
    min_happiness = detail.get("min_happiness")
    trade_species = trade_species_data.get("name")

    if min_level:
      return f"Lv{min_level}"
    if item:
      return item.replace("-", " ")
    if held_item:
      return held_item.replace("-", " ")
    if trade_species:
      return f"trade:{trade_species.replace('-', ' ')}"
    if min_happiness:
      return "なつき"
    if trigger:
      return trigger.replace("-", " ")
    return "special"


def fetch_evolutions(client: ApiClient, pokemon_id: int) -> list[dict[str, str]]:
    species = client.api(f"/pokemon-species/{pokemon_id}")
    chain_url = species["evolution_chain"]["url"]
    chain = client.get_json(chain_url)
    flat_rows: list[tuple[str, list[dict[str, Any]], list[Any]]] = []
    flatten_chain(chain["chain"], flat_rows)

    species_slug = species["name"]
    evolution_rows: list[dict[str, str]] = []
    sort_order = 10

    for node_name, _details, evolves_to in flat_rows:
        if node_name != species_slug:
            continue
        for child in evolves_to:
            details = child.get("evolution_details", [])
            if details:
                label = describe_evolution(details[0])
                evolution_rows.append(
                    {
                        "pokemon_id": str(pokemon_id),
                        "sort_order": str(sort_order),
                        "condition": label,
                    }
                )
                sort_order += 10
        break

    return evolution_rows


def write_csv(path: Path, fieldnames: list[str], rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--max-id", type=int, default=151)
    parser.add_argument("--out-dir", default=str(OUT_DIR))
    args = parser.parse_args()
    out_dir = Path(args.out_dir)

    client = ApiClient()

    locations: dict[str, dict[str, Any]] = {}
    encounters_rows: list[dict[str, str]] = []
    evolutions_rows: list[dict[str, str]] = []
    level_rows: list[dict[str, str]] = []
    machine_rows: list[dict[str, str]] = []

    for pokemon_id in range(1, args.max_id + 1):
        encounter_part, location_part = fetch_encounters(client, pokemon_id)
        encounters_rows.extend(encounter_part)
        locations.update(location_part)

        level_part, machine_part = fetch_level_up_and_machines(client, pokemon_id)
        level_rows.extend(level_part)
        machine_rows.extend(machine_part)

        evolutions_rows.extend(fetch_evolutions(client, pokemon_id))
        print(f"fetched {pokemon_id:04d}", flush=True)

    location_rows = sorted(locations.values(), key=lambda row: (int(row["order"]), row["name"]))

    write_csv(
        out_dir / "locations.csv",
        ["location_id", "name", "order", "postgame_only"],
        [{k: str(v) for k, v in row.items()} for row in location_rows],
    )
    write_csv(
        out_dir / "encounters.csv",
        ["location_id", "pokemon_id", "method", "rate", "min_level", "max_level", "postgame_only"],
        encounters_rows,
    )
    write_csv(
        out_dir / "evolutions.csv",
        ["pokemon_id", "sort_order", "condition"],
        evolutions_rows,
    )
    write_csv(
        out_dir / "level_up_moves.csv",
        ["pokemon_id", "sort_order", "level", "move"],
        level_rows,
    )
    write_csv(
        out_dir / "machines.csv",
        ["pokemon_id", "sort_order", "kind", "move"],
        machine_rows,
    )

    print(f"wrote locations={len(location_rows)}")
    print(f"wrote encounters={len(encounters_rows)}")
    print(f"wrote evolutions={len(evolutions_rows)}")
    print(f"wrote level_up_moves={len(level_rows)}")
    print(f"wrote machines={len(machine_rows)}")


if __name__ == "__main__":
    main()
