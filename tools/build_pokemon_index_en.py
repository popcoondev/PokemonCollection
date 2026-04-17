#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
INDEX_JA_PATH = ROOT / "data" / "sd" / "pokemon" / "index.json"
INDEX_EN_PATH = ROOT / "data" / "sd" / "pokemon" / "index_en.json"
API_BASE = "https://pokeapi.co/api/v2"
USER_AGENT = "PokemonCollection/1.0 (+english index builder)"


class ApiClient:
    def __init__(self, throttle_sec: float = 0.0) -> None:
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
        if self.throttle_sec > 0:
            time.sleep(self.throttle_sec)
        return payload

    def api(self, path: str) -> Any:
        return self.get_json(f"{API_BASE}{path}")


def load_index(path: Path) -> list[dict[str, Any]]:
    with path.open("r", encoding="utf-8") as f:
        payload = json.load(f)
    if not isinstance(payload, list):
        raise RuntimeError(f"unexpected index payload at {path}")
    return payload


def pick_english_name(species_payload: dict[str, Any]) -> str:
    for entry in species_payload.get("names", []):
        if entry.get("language", {}).get("name") == "en":
            name = entry.get("name")
            if isinstance(name, str) and name:
                return name

    fallback = species_payload.get("name")
    if isinstance(fallback, str) and fallback:
        return fallback.replace("-", " ").title()
    raise RuntimeError(f"english name not found for species {species_payload.get('id')}")


def build_index_en(client: ApiClient, index_ja: list[dict[str, Any]]) -> list[dict[str, Any]]:
    results: list[dict[str, Any]] = []
    for entry in index_ja:
        pokemon_id = int(entry.get("id", 0))
        if pokemon_id <= 0:
            continue
        species = client.api(f"/pokemon-species/{pokemon_id}/")
        results.append({
            "id": pokemon_id,
            "n": pick_english_name(species),
        })
        print(f"{pokemon_id:04d}: {results[-1]['n']}", flush=True)
    return results


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--throttle-sec", type=float, default=0.0)
    args = parser.parse_args()

    index_ja = load_index(INDEX_JA_PATH)
    client = ApiClient(throttle_sec=args.throttle_sec)
    index_en = build_index_en(client, index_ja)

    INDEX_EN_PATH.parent.mkdir(parents=True, exist_ok=True)
    with INDEX_EN_PATH.open("w", encoding="utf-8") as f:
        json.dump(index_en, f, ensure_ascii=False, indent=2)
        f.write("\n")

    print(f"wrote {INDEX_EN_PATH} ({len(index_en)} entries)")


if __name__ == "__main__":
    main()
