#!/usr/bin/env python3
from __future__ import annotations

import csv
import json
import argparse
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "data_src" / "firered"
OUT_DIR = ROOT / "data" / "sd" / "pokemon" / "firered"
OUT_POKEMON_DIR = OUT_DIR / "pokemon"
OUT_LOCATION_MAP_PATH = OUT_DIR / "location_pokemon_map.json"


def read_csv(name: str) -> list[dict[str, str]]:
    path = SRC_DIR / name
    with path.open("r", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def as_bool(value: str) -> bool:
    return str(value).strip().lower() in {"1", "true", "yes", "on"}


def as_int(value: str, default: int = 0) -> int:
    text = str(value).strip()
    return int(text) if text else default


def build_locations() -> tuple[list[dict], dict[str, dict]]:
    location_rows = read_csv("locations.csv")
    encounter_rows = read_csv("encounters.csv")

    pokemon_by_location: dict[str, set[int]] = defaultdict(set)
    for row in encounter_rows:
        pokemon_by_location[row["location_id"]].add(as_int(row["pokemon_id"]))

    ordered_locations: list[dict] = []
    location_index: dict[str, dict] = {}
    for row in sorted(location_rows, key=lambda r: as_int(r["order"])):
        entry = {
            "id": row["location_id"],
            "name": row["name"],
            "order": as_int(row["order"]),
            "postgame_only": as_bool(row["postgame_only"]),
            "pokemon": sorted(pokemon_by_location.get(row["location_id"], set())),
        }
        ordered_locations.append(entry)
        location_index[row["location_id"]] = entry

    return ordered_locations, location_index


def build_pokemon_payloads(location_index: dict[str, dict]) -> dict[int, dict]:
    encounter_rows = read_csv("encounters.csv")
    evolution_rows = read_csv("evolutions.csv")
    level_move_rows = read_csv("level_up_moves.csv")
    machine_rows = read_csv("machines.csv")

    payloads: dict[int, dict] = defaultdict(
        lambda: {
            "id": 0,
            "locations": [],
            "evolution": [],
            "level_up_moves": [],
            "machines": [],
            "hms": [],
        }
    )

    for row in encounter_rows:
        pokemon_id = as_int(row["pokemon_id"])
        payload = payloads[pokemon_id]
        payload["id"] = pokemon_id
        location = location_index.get(row["location_id"])
        payload["locations"].append(
            {
                "area": location["name"] if location else row["location_id"],
                "method": row["method"],
                "rate": row["rate"],
                "min_level": as_int(row["min_level"]),
                "max_level": as_int(row["max_level"]),
                "postgame_only": as_bool(row["postgame_only"]),
            }
        )

    for row in sorted(evolution_rows, key=lambda r: (as_int(r["pokemon_id"]), as_int(r["sort_order"]))):
        pokemon_id = as_int(row["pokemon_id"])
        payload = payloads[pokemon_id]
        payload["id"] = pokemon_id
        payload["evolution"].append(row["condition"])

    for row in sorted(level_move_rows, key=lambda r: (as_int(r["pokemon_id"]), as_int(r["sort_order"]))):
        pokemon_id = as_int(row["pokemon_id"])
        payload = payloads[pokemon_id]
        payload["id"] = pokemon_id
        payload["level_up_moves"].append(
            {
                "level": as_int(row["level"]),
                "name": row["move"],
            }
        )

    for row in sorted(machine_rows, key=lambda r: (as_int(r["pokemon_id"]), as_int(r["sort_order"]))):
        pokemon_id = as_int(row["pokemon_id"])
        payload = payloads[pokemon_id]
        payload["id"] = pokemon_id
        target_key = "hms" if row["kind"].strip().lower() == "hm" else "machines"
        payload[target_key].append(row["move"])

    return dict(sorted(payloads.items()))


def write_json(path: Path, payload) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--max-id", type=int, default=386)
    args = parser.parse_args()

    locations, location_index = build_locations()
    pokemon_payloads = build_pokemon_payloads(location_index)

    write_json(
        OUT_DIR / "locations.json",
        [
            {
                "id": loc["id"],
                "name": loc["name"],
                "order": loc["order"],
                "postgame_only": loc["postgame_only"],
                "pokemon": loc["pokemon"],
            }
            for loc in locations
        ],
    )

    write_json(
        OUT_LOCATION_MAP_PATH,
        {
            loc["id"]: {
                "name": loc["name"],
                "order": loc["order"],
                "postgame_only": loc["postgame_only"],
                "pokemon": loc["pokemon"],
            }
            for loc in locations
        },
    )

    for pokemon_id in range(1, args.max_id + 1):
        payload = pokemon_payloads.get(
            pokemon_id,
            {
                "id": pokemon_id,
                "locations": [],
                "evolution": [],
                "level_up_moves": [],
                "machines": [],
                "hms": [],
            },
        )
        write_json(OUT_POKEMON_DIR / f"{pokemon_id:04d}.json", payload)

    print(f"wrote {len(locations)} locations")
    print("wrote location_pokemon_map.json")
    print(f"wrote {args.max_id} pokemon files")


if __name__ == "__main__":
    main()
