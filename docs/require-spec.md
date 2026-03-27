M5Stack CoreS3 ポケモン図鑑 要件定義書
1. システム概要
本システムは、M5Stack CoreS3を用いて、外部SDカードに保存されたポケモンデータを閲覧するための専用端末ソフトウェアである。子供が直感的に操作できるよう、タッチパネルを活用したUIと、1画面1情報のレイアウトを特徴とする。
2. 開発環境・使用ライブラリ
OS: macOS / VSCode + PlatformIO
Framework: Arduino / ESP-IDF
Main Library:
M5Unified: CoreS3ハードウェア制御、タッチパネル取得
LovyanGFX: 高速グラフィック描画（M5Unified同梱）
ArduinoJson: SDカードからのJSONデータパース
PNGdec / JPEGDEC: 画像デコード用
3. データ取得戦略 (Data Acquisition)
公式サイト（zukan.pokemon.co.jp）の動的な描画を確実に回避し、100%正確なデータを取得するための戦略。
3.1 抽出手法
JSON Direct Extraction: HTMLソース内の末尾にある <script id="json-data" type="application/json"> タグを抽出する。
メリット:
JavaScriptの実行（Swiperの描画など）を待つ必要がない。
クラス名の変更や、スライダーのクローン要素による誤取得を物理的に排除できる。
タイプ名などがIDで管理されているため、正確なマッピングが可能。
3.2 正規化ルール
ID: 常に4桁のゼロ埋め文字列（例: 0001）として管理。
タイプ: 公式のタイプID（1〜18）を日本語名に変換して保持。
画像: プロトコル相対パス（//）を https: に補完。
4. データフォーマット定義
4.1 検索用インデックス (/pokemon/index.json)
起動時にRAMへ読み込む軽量なリスト。
[
  {"id": 1, "n": "フシギダネ"},
  {"id": 2, "n": "フシギソウ"}
]


4.2 個別詳細データ (/pokemon/details/{id}.json)
{
  "id": 1,
  "name": "フシギダネ",
  "types": ["くさ", "どく"],
  "category": "たねポケモン",
  "height": "0.7m",
  "weight": "6.9kg",
  "abilities": [
    {"n": "しんりょく", "d": "ＨＰが　減ったとき　くさタイプの　技の　威力が　上がる。"}
  ],
  "weaknesses": ["ほのお", "こおり", "ひこう", "エスパー"],
  "evolutions": [
    {"id": 1, "n": "フシギダネ"},
    {"id": 2, "n": "フシギソウ"}
  ],
  "desc": "うまれたときから　せなかに　ふしぎな　たねが　はえていて..."
}


5. ファイル構成 (SDカード)
/sd
  /pokemon
    index.json          (全体のインデックス)
    /details
      0001.json         (詳細データ)
    /imgs
      0001.jpg          (メイン画像: 200x200px)
    /icons
      0001.jpg          (進化アイコン: 64x64px)


6. UI/UX 機能要件 (320x240)
6.1 メイン画面 (Detail View)
Header: No.と名前。右端に検索（虫眼鏡）アイコン。
Tabs: 下部に6つのタブ（すがた、せつめい、とくせい、からだ、よわてん、しんか）。
Page Turn: 画面の左右端をタッチすることで前後のポケモンへ移動。
6.2 検索画面 (Drumroll Search)
4桁ドラムロール: 0001〜9999を選択。
リアルタイムプレビュー: 選択中の番号に該当するポケモンの画像と名前を表示。
おまかせボタン: ランダムに番号をシャッフル。
7. 実装上の技術的制約
PSRAM: CoreS3の8MB PSRAMを活用し、index.json と現在表示中の画像をキャッシュする。
日本語表示: M5.Lcd.loadFont を使用し、SDカードから日本語フォントを読み込む。
描画: スプライト（LGFX_Sprite）を使用したダブルバッファリング。
8. ステータス
[x] スクレイピング・プロトタイプ完了
[x] データ構造（JSON）の決定
[x] UI/UX シミュレーター（React）による検証完了
[ ] M5Stack CoreS3 C++ 実装開始
