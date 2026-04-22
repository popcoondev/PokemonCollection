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
cmake -S . -B build
cmake --build build
./build/pokemoncollection_sim
```

今後の予定:

1. `PcRenderer` を SDL 描画へ接続する
2. マウス/キーボードから `AppInput` を供給する
3. `AppState / update / render` を実機と共有する
4. 主要画面の表示確認を可能にする
