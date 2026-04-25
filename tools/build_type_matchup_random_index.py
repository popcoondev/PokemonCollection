from __future__ import annotations

import json
from pathlib import Path


TYPE_NAMES_JA = [
    "ノーマル",
    "ほのお",
    "みず",
    "でんき",
    "くさ",
    "こおり",
    "かくとう",
    "どく",
    "じめん",
    "ひこう",
    "エスパー",
    "むし",
    "いわ",
    "ゴースト",
    "ドラゴン",
    "あく",
    "はがね",
    "フェアリー",
]


def main() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    details_dir = repo_root / "data" / "sd" / "pokemon" / "details"
    output_path = repo_root / "data" / "sd" / "pokemon" / "type_matchup_random_index.json"

    type_map: dict[str, list[int]] = {name: [] for name in TYPE_NAMES_JA}

    for path in sorted(details_dir.glob("*.json")):
        with path.open("r", encoding="utf-8") as fp:
            doc = json.load(fp)
        pokemon_id = int(doc.get("id", 0))
        for type_name in doc.get("types", []):
            if type_name in type_map:
                type_map[type_name].append(pokemon_id)

    payload = {
        "types": [type_map[name] for name in TYPE_NAMES_JA],
    }
    output_path.write_text(
        json.dumps(payload, ensure_ascii=False, separators=(",", ":")),
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
