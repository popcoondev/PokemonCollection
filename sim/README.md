# PC Simulator Scaffold

このディレクトリは PC シミュレータ向けの入口と実行基盤を置くための足場です。

現段階では以下だけを目的とします。

- 実機向けコードと分離した PC エントリポイントの置き場所を決める
- `Renderer` / `AppInput` の差し替え先を明確にする
- 将来の 320x240 仮想画面表示を追加しやすくする

今後の予定:

現在入っているもの:

- `sim/main.cpp`
  - SDL2 で 320x240 論理サイズの最小ウィンドウを開く
- `sim/CMakeLists.txt`
  - SDL2 前提の最小ビルド定義

ビルド例:

```bash
cd sim
./bootstrap.sh
cmake -S . -B build
cmake --build build
./build/pokemoncollection_sim
```

preset を使う場合:

```bash
cd sim
./bootstrap.sh default
cmake --build --preset default
./build/pokemoncollection_sim
```

`ArduinoJson` は次の順で解決されます。

1. `sim/local-deps/ArduinoJson`
2. `pio run -e m5stack-cores3` で展開された `.pio/libdeps/.../ArduinoJson/src`
3. `ARDUINOJSON_ROOT` で指定したローカル配置先
4. CMake `FetchContent` による自動取得

ネットワークを使わずにビルドしたい場合は、`ArduinoJson` の配置先を明示してください。

```bash
ARDUINOJSON_ROOT=/path/to/ArduinoJson ./bootstrap.sh default
```

`bootstrap.sh` は外部の `ArduinoJson` を見つけたら `sim/local-deps/ArduinoJson` にコピーして、次回以降はそのローカルキャッシュを優先利用します。

Apple Silicon で既存の `build` ディレクトリが `x86_64` 向けに作られている場合は、別ディレクトリで作り直してください。

```bash
./bootstrap.sh macos-arm64
cmake --build --preset macos-arm64
./build-arm64/pokemoncollection_sim
```

必要な依存:

- `cmake`
- `sdl2`
- `sdl2_image`
- `sdl2_ttf`
- `ArduinoJson` のローカル配置、またはネットワーク経由の自動取得

macOS + Homebrew なら最低限これで揃います。

```bash
brew install cmake sdl2 sdl2_image sdl2_ttf
```

今後の予定:

1. `PcRenderer` を SDL 描画へ接続する
2. マウス/キーボードから `AppInput` を供給する
3. `AppState / update / render` を実機と共有する
4. 主要画面の表示確認を可能にする
