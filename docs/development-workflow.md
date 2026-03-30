# 開発運用ルール

## 目的

このドキュメントは、feature 開発から PR までの進め方を固定し、履歴と設計変更を追いやすくするためのものです。

## 基本ルール

- すべての作業は最新の `main` から feature ブランチを切って始める
- 1つの feature ブランチでは、原則として 1つの課題群だけを扱う
- 実装後は build と実機確認をしてから commit する
- push 後は PR を作成し、マージ後に次の feature へ進む
- 次の feature ブランチは、必ず最新の `main` から切り直す

## ブランチ運用

推奨手順:

1. `git fetch origin`
2. `git checkout main`
3. `git pull --ff-only origin main`
4. `git checkout -b feature/<topic>`

ルール:

- feature から feature を生やさない
- 途中実験の branch は main へ混ぜない
- 既存 branch が途中で崩れた場合は、必要に応じて `origin/main` から切り直す

## issue 運用

課題は `docs/issues.md` を正本として扱います。

ルール:

- 1課題 1項目で記録する
- `状態` は `open / doing / done`
- 修正後は `対応メモ` を更新する
- GitHub issue を作った場合も、ローカルの `docs/issues.md` に取り込む

大きい課題は分割します。

例:

- 基盤整理
- 画面ごとの載せ替え
- キャッシュ導入

## ドキュメント更新ルール

以下のどれかに当てはまる変更は、コード変更と同じ PR でドキュメントも更新します。

- 新しいアーキテクチャを導入した
- 既存の責務分離を変えた
- UI の基本操作や画面遷移を変えた
- 開発フローや issue 管理ルールを変えた

更新対象:

- 設計変更: `docs/architecture.md`
- 課題管理: `docs/issues.md`
- 要件の追記・修正: `docs/require-spec.md`
- 開発手順の変更: `docs/development-workflow.md`

## PR の最小チェック

PR 前に最低限確認する項目:

- `pio run -j1` が通る
- 実機で対象機能が再現・改善確認できている
- 既知の副作用があれば `docs/issues.md` に残している
- 設計変更がある場合、関連ドキュメントを更新している

## コミット方針

- コミットは feature 単位で意味が通る粒度にする
- 実験途中の壊れた状態は commit しない
- 文書更新だけでも意味がある場合は独立コミットでよい

## PR のまとめ方

PR では最低限これを説明します。

- 何を直したか
- どの issue に対応したか
- どこまでを今回のスコープにしたか
- 次の課題に何が残っているか

## 今後の前提

画像描画まわりは段階的に新パイプラインへ寄せます。

順序:

1. `すがた` タブ
2. プレビュー画面
3. 進化タブ
4. キャッシュと先読み

この順を崩す場合は、理由を `docs/architecture.md` または `docs/issues.md` に残します。
