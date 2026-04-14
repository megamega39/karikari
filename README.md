# karikari

C++ + Win32 API + Direct2D 製の画像/動画ビューア

## 特徴

- **高速描画** — Direct2D + WIC によるハードウェアアクセラレーション描画
- **書庫対応** — ZIP / 7z / RAR / CBZ / CBR / CB7 をそのまま閲覧
- **動画再生** — mpv + MediaFoundation による動画・音声再生
- **見開き表示** — 自動 / 単独 / 見開きモード、綴じ方向切替
- **プリフェッチ** — 前後のページをバックグラウンドで先読み
- **本棚・お気に入り・履歴** — よく見るフォルダや書庫を素早くアクセス
- **カスタムキーバインド** — ショートカットキーを自由に変更可能
- **多言語対応** — 日本語・英語

## ダウンロード

[最新リリース](https://github.com/megamega39/karikari/releases/latest)からダウンロードできます。

| ファイル | 説明 |
|---------|------|
| `karikari-x.x.x-setup.exe` | インストーラー版（スタートメニュー登録・アンインストーラー付き） |
| `karikari-x.x.x-portable.zip` | ポータブル版（展開してすぐ使える） |

## 対応フォーマット

| 種類 | フォーマット |
|------|------------|
| 画像 | JPEG, PNG, BMP, GIF, WebP, AVIF, TIFF, ICO |
| 動画 | MP4, MKV, AVI, MOV, WMV, WebM, FLV, M4V, TS, MPG |
| 音声 | MP3, WAV, OGG, FLAC, M4A, AAC, WMA, Opus |
| 書庫 | ZIP, 7z, RAR (v4/v5), CBZ, CBR, CB7 |

## ショートカットキー

### ナビゲーション

| キー | 動作 |
|------|------|
| `←` / `→` | 前 / 次のページ |
| `↑` / `↓` | 前 / 次の書庫（書庫モード時） |
| `Home` / `End` | 最初 / 最後のページ |
| `Alt+←` / `Alt+→` | 戻る / 進む |
| `Alt+↑` | 親フォルダへ |
| `Enter` | 開く |

### 表示

| キー | 動作 |
|------|------|
| `W` | ウィンドウに合わせる |
| `1` / `2` / `3` | 単独 / 見開き / 自動 |
| `B` | 綴じ方向切替 |
| `F11` | 全画面 |
| `G` | グリッド表示 |
| `P` | ホバープレビュー |

### ズーム

| キー | 動作 |
|------|------|
| `Ctrl++` / `Ctrl+-` | 拡大 / 縮小（25%刻み） |
| `Ctrl+ホイール` | なめらかズーム |
| `Ctrl+0` | ズームリセット |

### その他

| キー | 動作 |
|------|------|
| `Space` | 再生 / 一時停止 |
| `Ctrl+C` | 画像コピー |
| `Ctrl+R` | 回転（時計回り） |
| `F5` | 更新 |
| `F1` | ヘルプ |
| `C` | 本棚 |
| `H` | 履歴 |

すべてのキーバインドは設定から変更できます。

## ビルド

### 要件

- Windows 10 以降
- Visual Studio 2022（MSVC、C++17）
- CMake 3.20 以上

### 手順

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

実行には以下のDLLが必要です（exeと同じフォルダに配置）:

- `7z.dll` — [7-Zip](https://www.7-zip.org/) から取得（または `C:\Program Files\7-Zip` にインストール済みなら自動検出）
- `libmpv-2.dll` — [mpv](https://mpv.io/) から取得

## ライセンス

[GPL-3.0](LICENSE)

---

# karikari (English)

An image and video viewer built with C++ + Win32 API + Direct2D.

## Features

- **Fast rendering** — Hardware-accelerated drawing with Direct2D + WIC
- **Archive support** — Browse ZIP / 7z / RAR / CBZ / CBR / CB7 directly
- **Video playback** — Play video and audio via mpv + MediaFoundation
- **Spread view** — Auto / single / spread page modes with binding direction toggle
- **Prefetch** — Background preloading of adjacent pages
- **Bookshelf, favorites & history** — Quick access to frequently viewed folders and archives
- **Custom keybindings** — Fully configurable keyboard shortcuts
- **Multilingual** — Japanese and English

## Download

Download from the [latest release](https://github.com/megamega39/karikari/releases/latest).

| File | Description |
|------|-------------|
| `karikari-x.x.x-setup.exe` | Installer (Start menu entry, uninstaller included) |
| `karikari-x.x.x-portable.zip` | Portable (extract and run) |

## Supported Formats

| Type | Formats |
|------|---------|
| Image | JPEG, PNG, BMP, GIF, WebP, AVIF, TIFF, ICO |
| Video | MP4, MKV, AVI, MOV, WMV, WebM, FLV, M4V, TS, MPG |
| Audio | MP3, WAV, OGG, FLAC, M4A, AAC, WMA, Opus |
| Archive | ZIP, 7z, RAR (v4/v5), CBZ, CBR, CB7 |

## Keyboard Shortcuts

### Navigation

| Key | Action |
|-----|--------|
| `←` / `→` | Previous / next page |
| `↑` / `↓` | Previous / next archive (in archive mode) |
| `Home` / `End` | First / last page |
| `Alt+←` / `Alt+→` | Back / forward |
| `Alt+↑` | Go to parent folder |
| `Enter` | Open |

### View

| Key | Action |
|-----|--------|
| `W` | Fit to window |
| `1` / `2` / `3` | Single / spread / auto |
| `B` | Toggle binding direction |
| `F11` | Fullscreen |
| `G` | Grid view |
| `P` | Hover preview |

### Zoom

| Key | Action |
|-----|--------|
| `Ctrl++` / `Ctrl+-` | Zoom in / out (25% steps) |
| `Ctrl+Wheel` | Smooth zoom |
| `Ctrl+0` | Reset zoom |

### Other

| Key | Action |
|-----|--------|
| `Space` | Play / pause |
| `Ctrl+C` | Copy image |
| `Ctrl+R` | Rotate (clockwise) |
| `F5` | Refresh |
| `F1` | Help |
| `C` | Bookshelf |
| `H` | History |

All keybindings can be customized in settings.

## Build

### Requirements

- Windows 10 or later
- Visual Studio 2022 (MSVC, C++17)
- CMake 3.20+

### Steps

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The following DLLs are required at runtime (place next to the exe):

- `7z.dll` — From [7-Zip](https://www.7-zip.org/) (or auto-detected if 7-Zip is installed at `C:\Program Files\7-Zip`)
- `libmpv-2.dll` — From [mpv](https://mpv.io/)

## License

[GPL-3.0](LICENSE)
