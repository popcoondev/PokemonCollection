# FireRedデータ生成フロー

## 方針
- FireRed攻略データは手書き JSON を増やさず、`CSV -> JSON` の生成フローで管理する
- SD に置く実データは生成物とし、編集元は `data_src/firered/` に集約する
- ポケモン別 JSON と場所一覧 JSON は同じ入力 CSV 群からまとめて生成する

## ディレクトリ
- raw取得:
  - `data_src/firered_raw/locations.csv`
  - `data_src/firered_raw/encounters.csv`
  - `data_src/firered_raw/evolutions.csv`
  - `data_src/firered_raw/level_up_moves.csv`
  - `data_src/firered_raw/machines.csv`
- curated入力:
  - `data_src/firered/location_master.csv`
  - `data_src/firered/method_master.csv`
  - `data_src/firered/move_name_map.csv`
  - `data_src/firered/move_name_map_generated.csv`
  - `data_src/firered/item_name_map.csv`
  - `data_src/firered/item_name_map_generated.csv`
  - `data_src/firered/locations.csv`
  - `data_src/firered/encounters.csv`
  - `data_src/firered/evolutions.csv`
  - `data_src/firered/level_up_moves.csv`
  - `data_src/firered/machines.csv`
- 生成先:
  - `data/sd/pokemon/firered/locations.json`
  - `data/sd/pokemon/firered/pokemon/%04d.json`

## 役割

### `locations.csv`
- 場所一覧のマスタ
- `order` でゲーム進行順を管理する
- `postgame_only` で殿堂入り後限定を管理する

カラム:
- `location_id`
- `name`
- `order`
- `postgame_only`

### `encounters.csv`
- 場所とポケモンの出現対応
- `しゅつげん` タブと `場所からみる` の両方の元になる

カラム:
- `location_id`
- `pokemon_id`
- `method`
- `rate`
- `min_level`
- `max_level`
- `postgame_only`

### `evolutions.csv`
- FireRed向けの進化条件文字列

カラム:
- `pokemon_id`
- `sort_order`
- `condition`

### `level_up_moves.csv`
- レベル技一覧

カラム:
- `pokemon_id`
- `sort_order`
- `level`
- `move`

### `machines.csv`
- `kind` が `machine` のものは `マシン`
- `kind` が `hm` のものは `ひでん`

カラム:
- `pokemon_id`
- `sort_order`
- `kind`
- `move`

## 生成スクリプト
- `tools/build_firered_data.py`
- 入力 CSV を読み、ポケモン別 JSON と場所一覧 JSON を再生成する

実行例:

```bash
python3 tools/build_firered_data.py
```

## 元データの一次ソース候補
- 主取得元: PokéAPI
  - 出現場所
  - レベル技
  - TM/HM
  - 進化条件
- 補助確認:
  - Bulbapedia FRLG locations
  - Serebii FRLG / RS Dex

## 自動取得スクリプト
- `tools/import_firered_from_pokeapi.py`
- PokéAPI から以下の raw CSV を自動生成する
  - `data_src/firered_raw/locations.csv`
  - `data_src/firered_raw/encounters.csv`
  - `data_src/firered_raw/evolutions.csv`
  - `data_src/firered_raw/level_up_moves.csv`
  - `data_src/firered_raw/machines.csv`

## curated変換スクリプト
- `tools/curate_firered_data.py`
- raw CSV を FireRed 用に絞り込み、`data_src/firered/` の最終CSVへ変換する
- `location_master.csv` にある enabled 場所だけを残す
- `method_master.csv` にある enabled 出現方法だけを残す
- `move_name_map.csv` で一部技名を日本語化する
- `item_name_map.csv` と generated map で進化条件の英語を日本語化する
- HM は `cut/surf/...` の固定リストで `ひでん` に分離する

## 日本語マップ生成スクリプト
- `tools/build_pokeapi_ja_maps.py`
- raw CSV に含まれる技名・進化アイテム名を PokéAPI から日本語化して生成する
- 出力先:
  - `data_src/firered/move_name_map_generated.csv`
  - `data_src/firered/item_name_map_generated.csv`

実行例:

```bash
python3 tools/import_firered_from_pokeapi.py --max-id 151
python3 tools/build_pokeapi_ja_maps.py
python3 tools/curate_firered_data.py
python3 tools/build_firered_data.py
```

### 取得範囲
- `--max-id 151`
  - カントー図鑑用
- `--max-id 386`
  - 全国図鑑用

## 運用ルール
- `data/sd/pokemon/firered/` は原則として手編集しない
- raw取得結果をそのまま使わない
- `location_master.csv` と `method_master.csv` で FireRed 用に絞り込む
- 必要に応じて `move_name_map.csv` と `item_name_map.csv` を増やす
- 最終的な手順は以下

```bash
python3 tools/import_firered_from_pokeapi.py --max-id 386
python3 tools/build_pokeapi_ja_maps.py
python3 tools/curate_firered_data.py
python3 tools/build_firered_data.py
```
