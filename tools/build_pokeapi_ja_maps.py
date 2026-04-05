#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
RAW_DIR = ROOT / "data_src" / "firered_raw"
CURATED_DIR = ROOT / "data_src" / "firered"
API_BASE = "https://pokeapi.co/api/v2"
USER_AGENT = "PokemonCollection/1.0 (+ja map builder)"
GENERIC_EVOLUTION_TOKENS = {
    "trade",
    "level up",
    "use move",
    "three critical hits",
    "なつき",
}


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


def pick_name(entries: list[dict[str, Any]]) -> str | None:
    for lang in ("ja-Hrkt", "ja", "en"):
        for entry in entries:
            if entry.get("language", {}).get("name") == lang:
                return entry.get("name")
    return None


def normalize_slug(text: str) -> str:
    return text.strip().lower().replace(" ", "-")


def load_existing_map(path: Path) -> dict[str, str]:
    return {row["raw_name"]: row["name_ja"] for row in read_csv(path) if row.get("name_ja")}


def collect_unique_moves() -> list[str]:
    values: set[str] = set()
    for name in ("level_up_moves.csv", "machines.csv"):
        for row in read_csv(RAW_DIR / name):
            move = row.get("move", "").strip()
            if move:
                values.add(move)
    return sorted(values)


def collect_unique_items() -> list[str]:
    values: set[str] = set()
    for row in read_csv(RAW_DIR / "evolutions.csv"):
        condition = row.get("condition", "").strip()
        if not condition or condition.startswith("Lv") or condition in GENERIC_EVOLUTION_TOKENS:
            continue
        values.add(condition)
    return sorted(values)


def build_move_map(client: ApiClient) -> list[dict[str, str]]:
    existing = load_existing_map(CURATED_DIR / "move_name_map.csv")
    existing.update(load_existing_map(CURATED_DIR / "move_name_map_generated.csv"))
    rows = []
    for raw_name in collect_unique_moves():
        if raw_name in existing:
            rows.append({"raw_name": raw_name, "name_ja": existing[raw_name]})
            continue
        payload = client.api(f"/move/{normalize_slug(raw_name)}")
        ja_name = pick_name(payload.get("names", [])) or raw_name
        rows.append({"raw_name": raw_name, "name_ja": ja_name})
        print(f"move {raw_name} -> {ja_name}", flush=True)
    return rows


def build_item_map(client: ApiClient) -> list[dict[str, str]]:
    existing = load_existing_map(CURATED_DIR / "item_name_map.csv")
    existing.update(load_existing_map(CURATED_DIR / "item_name_map_generated.csv"))
    rows = []
    for raw_name in collect_unique_items():
        if raw_name in existing:
            rows.append({"raw_name": raw_name, "name_ja": existing[raw_name]})
            continue
        try:
            payload = client.api(f"/item/{normalize_slug(raw_name)}")
            ja_name = pick_name(payload.get("names", [])) or raw_name
        except RuntimeError:
            ja_name = raw_name
            print(f"item {raw_name} -> SKIP", flush=True)
        rows.append({"raw_name": raw_name, "name_ja": ja_name})
        if ja_name != raw_name:
            print(f"item {raw_name} -> {ja_name}", flush=True)
    return rows


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--throttle-sec", type=float, default=0.65)
    args = parser.parse_args()

    client = ApiClient(throttle_sec=args.throttle_sec)
    move_rows = build_move_map(client)
    item_rows = build_item_map(client)

    write_csv(CURATED_DIR / "move_name_map_generated.csv", ["raw_name", "name_ja"], move_rows)
    write_csv(CURATED_DIR / "item_name_map_generated.csv", ["raw_name", "name_ja"], item_rows)

    print(f"wrote move_name_map_generated={len(move_rows)}")
    print(f"wrote item_name_map_generated={len(item_rows)}")


if __name__ == "__main__":
    main()
