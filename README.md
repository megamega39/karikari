# karikari

C++ + Direct2D 製の画像/動画ビューア for Windows

## 特徴

- **高速描画** — Direct2D によるハードウェアアクセラレーション描画
- **書庫対応** — ZIP / 7z / RAR / CBZ / CBR / CB7 をそのまま閲覧
- **動画再生** — 動画・音声ファイルの再生にも対応
- **見開き表示** — 自動 / 単独 / 見開きモード、綴じ方向切替
- **先読み** — 次のページをバックグラウンドで先読みしてスムーズに閲覧
- **本棚・お気に入り・履歴** — よく見るフォルダや書庫に素早くアクセス
- **キーボードショートカット** — 自由にカスタマイズ可能
- **多言語対応** — 日本語・英語

## ダウンロード

[最新リリース](https://github.com/megamega39/karikari/releases/latest)からダウンロードできます。

| ファイル | 説明 |
|---------|------|
| `karikari-x.x.x-setup.exe` | インストーラー版 |
| `karikari-x.x.x-portable.zip` | ポータブル版（展開してすぐ使える） |

## 対応フォーマット

| 種類 | フォーマット |
|------|------------|
| 画像 | JPEG, PNG, BMP, GIF, WebP, AVIF, TIFF, ICO |
| 動画 | MP4, MKV, AVI, MOV, WMV, WebM, FLV, M4V, TS, MPG |
| 音声 | MP3, WAV, OGG, FLAC, M4A, AAC, WMA, Opus |
| 書庫 | ZIP, 7z, RAR (v4/v5), CBZ, CBR, CB7 |

## 操作方法

| キー | 動作 |
|------|------|
| `←` `→` | 前 / 次のページ |
| `↑` `↓` | 前 / 次の書庫 |
| `Home` `End` | 最初 / 最後のページ |
| `Alt+←` `Alt+→` | 戻る / 進む |
| `Alt+↑` | 親フォルダへ |
| `W` | ウィンドウに合わせる |
| `1` `2` `3` | 単独 / 見開き / 自動 |
| `B` | 綴じ方向切替 |
| `F11` | 全画面 |
| `Ctrl+ホイール` | ズーム |
| `Space` | 再生 / 一時停止 |
| `Ctrl+C` | 画像コピー |
| `Ctrl+R` | 回転 |

すべてのキーは設定から変更できます。

## 動作環境

- Windows 10 以降

## ライセンス

[GPL-3.0](LICENSE)

---

# karikari (English)

An image and video viewer for Windows, built with C++ and Direct2D.

## Features

- **Fast rendering** — Hardware-accelerated drawing with Direct2D
- **Archive support** — Browse ZIP / 7z / RAR / CBZ / CBR / CB7 directly
- **Video playback** — Play video and audio files
- **Spread view** — Auto / single / spread page modes with binding direction toggle
- **Prefetch** — Background preloading for smooth browsing
- **Bookshelf, favorites & history** — Quick access to frequently viewed folders and archives
- **Custom keybindings** — Fully configurable keyboard shortcuts
- **Multilingual** — Japanese and English

## Download

Download from the [latest release](https://github.com/megamega39/karikari/releases/latest).

| File | Description |
|------|-------------|
| `karikari-x.x.x-setup.exe` | Installer |
| `karikari-x.x.x-portable.zip` | Portable (extract and run) |

## Supported Formats

| Type | Formats |
|------|---------|
| Image | JPEG, PNG, BMP, GIF, WebP, AVIF, TIFF, ICO |
| Video | MP4, MKV, AVI, MOV, WMV, WebM, FLV, M4V, TS, MPG |
| Audio | MP3, WAV, OGG, FLAC, M4A, AAC, WMA, Opus |
| Archive | ZIP, 7z, RAR (v4/v5), CBZ, CBR, CB7 |

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `←` `→` | Previous / next page |
| `↑` `↓` | Previous / next archive |
| `Home` `End` | First / last page |
| `Alt+←` `Alt+→` | Back / forward |
| `Alt+↑` | Go to parent folder |
| `W` | Fit to window |
| `1` `2` `3` | Single / spread / auto |
| `B` | Toggle binding direction |
| `F11` | Fullscreen |
| `Ctrl+Wheel` | Zoom |
| `Space` | Play / pause |
| `Ctrl+C` | Copy image |
| `Ctrl+R` | Rotate |

All keybindings can be customized in settings.

## System Requirements

- Windows 10 or later

## License

[GPL-3.0](LICENSE)
