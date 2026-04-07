# 実装ロードマップ

## Phase 1: 基盤
- [x] プロジェクト構造作成
- [x] ウィンドウ表示 + Direct2D初期化
- [x] 画像ファイルD&D → WICデコード → Direct2D表示
- [x] ホイールでページめくり

## Phase 2: 基本操作
- [x] フォルダ内の画像一覧 + 前後移動
- [x] ズーム（Ctrl+ホイール）+ パン（ドラッグ）
- [x] 見開き表示
- [x] スケールモード（Fit Window / Fit Width / Fit Height / Original）

## Phase 3: UI
- [x] フォルダツリー（TreeView）
- [x] ファイルリスト（ListView）
- [x] ナビゲーションバー（戻る/進む/上へ）
- [x] アドレスバー + ブレッドクラム

## Phase 4: 書庫対応
- [x] 書庫展開（7z.dll）
- [x] 書庫内画像表示

## Phase 5: 高速化
- [x] プリフェッチ（スレッドプール）
- [x] 画像キャッシュ（LRU、ID2D1Bitmap）
- [x] YCbCr最適化（ID2D1ImageSourceFromWic）

## Phase 6: 機能追加
- [x] お気に入り
- [x] 本棚（お気に入りに統合）
- [x] 履歴
- [x] 設定画面
- [x] 全画面表示
- [x] キーボードショートカット
- [x] 多言語対応（i18n）
- [x] コンテキストメニュー（ビューアー/ファイルリスト/ツリー）
- [x] Ctrl+C 画像クリップボードコピー
- [x] Delete ファイル削除
- [x] F2 ファイル名変更
- [x] ファイルリストフィルター
- [x] 動画・音声対応（MediaFoundation + libmpv）
