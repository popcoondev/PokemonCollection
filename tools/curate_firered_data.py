#!/usr/bin/env python3
from __future__ import annotations

import csv
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
RAW_DIR = ROOT / "data_src" / "firered_raw"
CURATED_DIR = ROOT / "data_src" / "firered"

HM_MOVES = {
    "cut",
    "fly",
    "surf",
    "strength",
    "flash",
    "rock smash",
    "waterfall",
    "dive",
}
GENERIC_EVOLUTION_MAP = {
    "trade": "こうかん",
    "level up": "レベルアップ",
    "use move": "とくしゅじょうけん",
    "three critical hits": "とくしゅじょうけん",
}
UNSUPPORTED_RAW_EVOLUTION_CONDITIONS = {
    "level up",
    "use move",
    "three critical hits",
    "black augurite",
    "strong style move",
    "peat block",
    "agile style move",
    "shiny stone",
    "dusk stone",
    "dawn stone",
    "razor fang",
    "razor claw",
    "protector",
    "electirizer",
    "magmarizer",
    "dubious disc",
    "reaper cloth",
}


def read_csv(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        return []
    with path.open("r", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def write_csv(path: Path, fieldnames: list[str], rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def load_enabled_location_master() -> tuple[dict[str, dict[str, str]], list[dict[str, str]]]:
    rows = read_csv(CURATED_DIR / "location_master.csv")
    enabled_rows = []
    enabled_map: dict[str, dict[str, str]] = {}
    for row in rows:
        enabled = row.get("enabled", "false").strip().lower() in {"1", "true", "yes", "on"}
        if not enabled:
            continue
        enabled_rows.append(row)
        enabled_map[row["location_id"]] = row
    enabled_rows.sort(key=lambda row: (int(row.get("order", "9999") or "9999"), row["name_ja"]))
    return enabled_map, enabled_rows


def load_method_master() -> dict[str, dict[str, str]]:
    rows = read_csv(CURATED_DIR / "method_master.csv")
    return {row["raw_method"]: row for row in rows}


def load_move_name_map() -> dict[str, str]:
    rows = read_csv(CURATED_DIR / "move_name_map.csv")
    rows += read_csv(CURATED_DIR / "move_name_map_generated.csv")
    return {row["raw_name"]: row["name_ja"] for row in rows if row.get("name_ja")}


def load_item_name_map() -> dict[str, str]:
    rows = read_csv(CURATED_DIR / "item_name_map.csv")
    rows += read_csv(CURATED_DIR / "item_name_map_generated.csv")
    return {row["raw_name"]: row["name_ja"] for row in rows if row.get("name_ja")}


def curate_locations(location_rows: list[dict[str, str]], enabled_locations: dict[str, dict[str, str]]) -> list[dict[str, str]]:
    raw_ids = {row["location_id"] for row in location_rows}
    rows: list[dict[str, str]] = []
    for location_id, row in enabled_locations.items():
        if location_id not in raw_ids:
            continue
        rows.append(
            {
                "location_id": location_id,
                "name": row["name_ja"],
                "order": row["order"],
                "postgame_only": row["postgame_only"],
            }
        )
    return rows


def curate_encounters(
    encounter_rows: list[dict[str, str]],
    enabled_locations: dict[str, dict[str, str]],
    method_master: dict[str, dict[str, str]],
) -> list[dict[str, str]]:
    deduped: dict[tuple[str, str, str, str, str, str, str], dict[str, str]] = {}
    for row in encounter_rows:
        location_id = row["location_id"]
        if location_id not in enabled_locations:
            continue
        method_row = method_master.get(row["method"])
        if method_row:
            enabled = method_row.get("enabled", "false").strip().lower() in {"1", "true", "yes", "on"}
            if not enabled:
                continue
            method = method_row.get("display_name", row["method"])
        else:
            method = row["method"]
        curated_row = {
            "location_id": location_id,
            "pokemon_id": row["pokemon_id"],
            "method": method,
            "rate": row["rate"],
            "min_level": row["min_level"],
            "max_level": row["max_level"],
            "postgame_only": enabled_locations[location_id]["postgame_only"],
        }
        dedupe_key = (
            curated_row["location_id"],
            curated_row["pokemon_id"],
            curated_row["method"],
            curated_row["rate"],
            curated_row["min_level"],
            curated_row["max_level"],
            curated_row["postgame_only"],
        )
        deduped[dedupe_key] = curated_row
    rows = list(deduped.values())
    rows.sort(key=lambda row: (row["location_id"], int(row["pokemon_id"]), row["method"], int(row["min_level"] or "0")))
    return rows


def curate_level_up_moves(rows: list[dict[str, str]], move_name_map: dict[str, str]) -> list[dict[str, str]]:
    curated = []
    for row in rows:
        move = move_name_map.get(row["move"], row["move"])
        curated.append({**row, "move": move})
    return curated


def curate_machines(rows: list[dict[str, str]], move_name_map: dict[str, str]) -> list[dict[str, str]]:
    curated = []
    for row in rows:
        raw_move = row["move"]
        normalized_move = raw_move.strip().lower()
        kind = "hm" if normalized_move in HM_MOVES else "machine"
        move = move_name_map.get(raw_move, raw_move)
        curated.append({**row, "kind": kind, "move": move})
    return curated


def translate_evolution_condition(condition: str, item_name_map: dict[str, str]) -> str:
    if not condition:
        return condition
    if condition.startswith("Lv"):
        return condition
    if condition in GENERIC_EVOLUTION_MAP:
        return GENERIC_EVOLUTION_MAP[condition]
    return item_name_map.get(condition, condition)


def curate_evolutions(rows: list[dict[str, str]], item_name_map: dict[str, str]) -> list[dict[str, str]]:
    curated = []
    for row in rows:
        if row["condition"] in UNSUPPORTED_RAW_EVOLUTION_CONDITIONS:
            continue
        curated.append({**row, "condition": translate_evolution_condition(row["condition"], item_name_map)})
    return curated


def main() -> None:
    enabled_locations, ordered_location_rows = load_enabled_location_master()
    method_master = load_method_master()
    move_name_map = load_move_name_map()
    item_name_map = load_item_name_map()

    raw_locations = read_csv(RAW_DIR / "locations.csv")
    raw_encounters = read_csv(RAW_DIR / "encounters.csv")
    raw_evolutions = read_csv(RAW_DIR / "evolutions.csv")
    raw_level_moves = read_csv(RAW_DIR / "level_up_moves.csv")
    raw_machines = read_csv(RAW_DIR / "machines.csv")

    location_rows = curate_locations(raw_locations, enabled_locations)
    encounter_rows = curate_encounters(raw_encounters, enabled_locations, method_master)
    evolution_rows = curate_evolutions(raw_evolutions, item_name_map)
    level_rows = curate_level_up_moves(raw_level_moves, move_name_map)
    machine_rows = curate_machines(raw_machines, move_name_map)

    write_csv(
        CURATED_DIR / "locations.csv",
        ["location_id", "name", "order", "postgame_only"],
        location_rows,
    )
    write_csv(
        CURATED_DIR / "encounters.csv",
        ["location_id", "pokemon_id", "method", "rate", "min_level", "max_level", "postgame_only"],
        encounter_rows,
    )
    write_csv(
        CURATED_DIR / "evolutions.csv",
        ["pokemon_id", "sort_order", "condition"],
        evolution_rows,
    )
    write_csv(
        CURATED_DIR / "level_up_moves.csv",
        ["pokemon_id", "sort_order", "level", "move"],
        level_rows,
    )
    write_csv(
        CURATED_DIR / "machines.csv",
        ["pokemon_id", "sort_order", "kind", "move"],
        machine_rows,
    )

    print(f"enabled locations in master={len(ordered_location_rows)}")
    print(f"wrote curated locations={len(location_rows)}")
    print(f"wrote curated encounters={len(encounter_rows)}")
    print(f"wrote curated evolutions={len(evolution_rows)}")
    print(f"wrote curated level_up_moves={len(level_rows)}")
    print(f"wrote curated machines={len(machine_rows)}")


if __name__ == "__main__":
    main()
