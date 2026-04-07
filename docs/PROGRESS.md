# 開発進捗メモ

## 2026-04-04: プロジェクト初期構築
- プロジェクト構造を作成（CLAUDE.md, CMakeLists.txt, src/main.cpp）
- 技術スタック: C++17 + Win32 API + Direct2D + WIC
- 最速を目指す設計方針を決定
  - Direct2D + WIC統合（YCbCr最適化）がWindows画像ビューアの最速解
  - メモリマップドファイル、GPUデコードは不要と判断
  - libjpeg-turbo単体よ��WIC+D2D統合の方が画像表示トータルで速い

## 2026-04-04: UI骨格構築
- main.cppを分割: app.h, decoder, viewer, window, navbar, addressbar, statusbar, splitter
- leeyez_kaiと同等のUIレイアウトをWin32ネイティブで構築
  - ナビバー（ToolBar + Segoe Fluent Iconsアイコン）
  - アドレスバー（Static + Edit）
  - スプリッター（垂直/水平、ドラッグリサイズ対応）
  - 左サイドバー: TreeView + ListView（上下分割）
  - 右ビューアー: ビューアーツールバー + Direct2D描画（黒背景）
  - ステータスバー（4パート）
- 設計判断: グローバルAppState構造体でシンプルに開始、スプリッターはカスタムウィンドウクラスで再利用可能に

## 2026-04-04: フォルダナビゲーション + ページめくり + ズーム/パン
- 新規ファイル: filelist.cpp/h, tree.cpp/h, nav.cpp/h
- フ��ルダツリ���: ドライブ列挙、フォルダ遅延展開（TVN_ITEMEXPANDING）、選択でナビゲート
- ファイルリスト: フォルダ読み込み（FindFirstFile）、画像フィ���タ、ListView表示、ダブルクリックで開く
- ナビゲーション: NavigateTo（フォルダ/画像判定）、戻る/進む/上へ、履歴スタック
- ビュー��ー拡張:
  - ホイール → ページめくり、Ctrl+ホイール → ズーム（×1.1/÷1.1）
  - ドラッグでパン（画像がビューアーより大きい場合のみ）
  - スケールモード: FitWindow/FitWidth/FitHeight/Original
  - D&Dもフォルダ/画像ナビゲーション対応に変更
- ツールバーボタン全接続: ページ送り��ズーム、フィットモード、ナビバー
- 対応拡張子: jpg/jpeg/png/bmp/gif/webp/avif/tiff/ico（leeyez_kai準拠）

## 2026-04-04: 見開き表示 + 書庫対応 + ブレッドクラム
- 見開き表示:
  - 2枚の画像を左右に隙間なく中央配置（PaintSpread）
  - 3モード: 自動（taureader互換判定）/ 単独 / 強制見開き
  - RTL（右綴じ）対応、ビューアーツールバーにA/1/2/→ボタン
  - ページ送りステップが見開き時は2に
- 書庫対応（archive.cpp/h）:
  - 7z.dll を LoadLibrary + GetProcAddress で直接使用（IInArchive COM）
  - 対応形式: zip/7z/rar/cbz/cbr/cb7
  - メモリ展開: ISequentialOutStream → std::vector<BYTE>
  - 書庫内パス形式: "archive.zip!folder/image.jpg"
  - decoder.cpp に DecodeImageFromMemory 追加（WIC CreateStream）
  - nav.cpp がフォルダ/書庫/書庫内画像を自動判定してナビゲート
  - 7z.dll 探索: exe同ディレクトリ → C:\Program Files\7-Zip\7z.dll
- ブレッドクラム（addressbar.cpp改善）:
  - パスを "> " 区切りで表示するStatic
  - クリックでEdit切替（直接パス入力可能）
  - Enter → ナビゲート、Esc → ブレッドクラムに戻る
  - サブクラス化でキー入力制御

## 2026-04-06: 画像切替・表示の高速化（計画#4完了）
- GetImageSize キャッシュをプリフェッチに統合
  - g_sizeCache に std::mutex 追加（スレッドセーフ化）
  - CacheImageSize() を viewer.h/cpp に公開
  - PrefetchWorker でデコード完了時に画像サイズをキャッシュに先読み
  - → ShouldShowSpread() での WIC デコーダ再オープンが不要に（20-60ms → 0ms）
- 既実装の高速化（前セッション）:
  - 見開き判定結果メモ化（g_spreadCache）
  - 書庫mutex改善（UI=lock_guard即座、worker=try_lock待たない）
  - プリフェッチ次ページ最優先投入
  - アニメーション判定の拡張子フィルタ（.gif/.webp以外スキップ）

## 2026-04-06: 全体最適化 (12件実装)
### 高優先度
- **#1** GoToFile() の重複線形探索統合 (nav.cpp): fileItems の2回O(n)探索→1回に
- **#2** viewableFiles O(1)検索化: NavState に viewableFileIndex 追加、filelist/nav で構築
- **#3** cache.cpp shared_mutex化: CacheGet を shared_lock（読み取り並行）、CachePut/Clear を unique_lock
- **#4** WebP magic byte チェック修正 (decoder.cpp): RIFF 4バイト→RIFF....WEBP 12バイト
- **#5** media WM_TIMER 最適化: 再生位置変化時のみ InvalidateRect

### 中優先度
- **#6** LayoutChildren() DPIキャッシュ (window.cpp): DPI変更時のみ再計算
- **#8** サムネイル生成バックグラウンド化 (filelist.cpp): UIスレッドブロック解消、WM_THUMB_DONE で差し替え
- **#10** UpdateStatusBar パート幅キャッシュ (statusbar.cpp): totalW変更時のみSB_SETPARTS

### 低優先度
- **#11** decoder.cpp 重複コード削減: LoadFirstFrame() 共通関数抽出
- **#12** prefetch.cpp memory_order最適化: relaxed/release に変更
- **#13** fswatcher.cpp バッファ 4KB→16KB に拡大
- **#14** media.cpp GDIブラシリーク防止: MediaShutdown() で DeleteObject()

### スキップ
- #7 (LoadSettings既にキャッシュ済み), #9 (SHGetFileInfo既にキャッシュ済み), #15 (コスト対効果低)

## 2026-04-06: ツリービューアイコンキャッシュのディスク永続化
- **問題**: 起動時に InitFolderTree() で SHGetFileInfoW を20-30回同期呼び出し → 6秒の遅延
- **原因**: g_iconIndexCache がメモリキャッシュのみで、起動のたびに全ミス
- **修正** (tree.cpp):
  - LoadIconCache(): icon_index.cache から TSV 形式でキャッシュ復元
  - SaveIconCache(): g_iconCacheDirty フラグで変更時のみディスク保存
  - InitFolderTree() の最初でロード、最後と ExpandTreeNode() 後にセーブ
  - 効果: 2回目以降の起動で SHGetFileInfoW をスキップ → 6秒→ほぼ0秒

## 2026-04-06: 追加最適化 (9件)
### 高優先度
- **#1** GoToFile の書庫モード探索 O(1)化 (nav.cpp): fileItems[index]で直接取得
- **#2** プリフェッチで既キャッシュをスキップ (prefetch.cpp): CacheGet()で事前チェック
- **#3** GetCachedArchive のキャッシュヒット時の無駄な SaveEntryCache 削除 (archive.cpp)

### 中優先度
- **#4** CacheGet の二重ロック→単一排他ロックに簡素化 (cache.cpp)
- **#5** サムネイルリサイズ補間: scale<=0.5でFant使用 (filelist.cpp)
- **#6** HasSubfolders のディスク永続化 (tree.cpp): subfolders.cache に保存/復元
- **#7** NormalizePath の in-place 版追加 (archive.cpp): コピー削減

### 低優先度
- **#8** SelectTreePath のルート探索で tp 再計算を削除 (tree.cpp)
- **#9** g_sizeCache の500超過時: 半分線形削除→clear()に簡素化 (viewer.cpp)

## 2026-04-06: 画像切替の最高速化 (5件)
- **#1** ホイール/キー連続入力のデバウンス (viewer.cpp): 30msタイマーで中間フレームスキップ
- **#2** 見開き判定のフォールバック (viewer.cpp): GetImageSize キャッシュミス時にWICを開かず即false→UIブロック回避
- **#3** 書庫見開きの非同期化 (nav.cpp): 2枚目がキャッシュミスなら1枚目のみ即表示→2枚目はプリフェッチに委譲
- **#4** fileItems逆引き O(1)化 (nav.cpp, filelist.cpp, app.h): fileItemIndex追加で線形探索を排除
- **#5** 書庫モード時プリフェッチ数1.5倍 (prefetch.cpp): 書庫は展開コスト高いので先読み増加

## 2026-04-06: ECCレビュー指摘事項の修正 (10件)
### CRITICAL (6件)
- **C1** CacheGet を ComPtr返しに変更 (cache.cpp/h): use-after-free 防止、ロック保持中にAddRef
- **C2** 書庫エントリのパストラバーサル検証 (archive.cpp): IsValidEntryPath()で`../`、絶対パスを拒否
- **C3** メモリ展開サイズ上限256MB (archive.cpp): MemoryOutStream::WriteでE_OUTOFMEMORY返却
- **C4** DLL検索パス制限 (media.cpp): カレントDir検索のフォールバックLoadLibraryを削除
- **C5** PostMessage失敗時のIWICBitmapリーク防止 (prefetch.cpp): 失敗時にRelease()
- **C6** PrefetchCancel: WaitForThreadpoolWorkCallbacks(TRUE)で完了待ち (prefetch.cpp)

## 2026-04-07: window.cpp 分割リファクタリング
- window.cpp（約2830行）を4つのファイルに分割
  - **command.cpp/h**: HandleCommand, HandleKeyDown（コマンド/キーボードショートカット処理）
  - **hover_preview.cpp/h**: ホバープレビュー関連（キャッシュ、非同期デコード、ウィンドウ管理）
  - **context_menu.cpp/h**: コンテキストメニュー（ShowFileContextMenu, ShowContextMenu, CopyToClipboard, CopyImageToClipboard, ShowInExplorer, ShowInputDialog）
  - **viewer_toolbar.cpp/h**: ビューアーツールバー（CreateViewerToolbars, UpdateViewerToolbarState, LayoutViewerToolbarLabels, UpdatePageCounter, UpdateZoomLabel）
- window.cppは約1520行 → 約790行に削減
- 設計判断: 機能を変えずコードの移動のみ。ToggleFullscreenはwindow.cppに残してextern公開（全画面切り替えはレイアウトに密結合のため）
- context_menu.cppのToggleFullscreenはWM_TOGGLE_FULLSCREENメッセージ経由で呼び出し（循環依存回避）

## 2026-04-07: nav.cpp / viewer.cpp リファクタリング (Step 4-7)

### Step 4: nav.cpp のビューア操作を viewer.cpp に委譲
- ViewerSetBitmap() / ViewerResetView() を viewer.cpp に追加
- nav.cpp の ShowArchiveImage() 内の直接操作を ViewerSetBitmap() 呼び出しに置換
- GoToFile() 内の zoom/scroll リセットを ViewerResetView() に置換
- 設計判断: ビューア状態の操作を viewer.cpp に集約し、nav.cpp からの直接参照を削減

### Step 5: NavigateOptions 構造体
- app.h に NavigateOptions 構造体追加（updateHistory, syncTreeSelection）
- NavigateTo のシグネチャを NavigateOptions 引数に変更（デフォルト引数付き）
- NavigateBack/NavigateForward/ドロップダウン: `{ false, true }` で履歴更新なし
- ツリー選択からの NavigateTo: `{ true, false }` でツリー同期なし
- app.h から skipHistoryUpdate / skipTreeSelect を削除（全参照を置換済み）
- 設計判断: bool フラグの set/reset パターンは例外安全でない。構造体引数で状態汚染を排除

### Step 6: GoToFile() の分解
- 約180行の GoToFile() を4つのサブ関数に分解:
  - ResolveIndex(): ループナビゲーション判定
  - PlayMediaFile(): メディアファイル再生
  - DisplayImageFile(): 見開き判定 + 画像表示
  - SyncListViewAndStatus(): ListView選択同期 + ステータスバー + ツールバー + タイトルバー更新
- GoToFile() 本体は約15行に簡素化

### Step 7: ビルド改善
- CMakeLists.txt に PCH（プリコンパイル済みヘッダー）追加: `target_precompile_headers(karikari PRIVATE src/app.h)`
- app.h に IDM_HIST_DROPDOWN_BACK / IDM_HIST_DROPDOWN_FWD 定数追加
- window.cpp のマジックナンバー 2000/3000 を定数に置換

### HIGH (4件)
- **H1** DWORDオーバーフロー防止 (decoder.cpp): size>MAXDWORD時にE_INVALIDARG
- **H2** wrapNavigation設定の即時反映 (nav.cpp): NavResetSettings()追加、設定変更後に呼び出し
- **H3** デバイスロスト回復後の画像再表示 (viewer.cpp): GoToFile(currentIndex)で再デコード
- **H4** WM_PREFETCH_DONEのindex範囲チェック (window.cpp): 既に実装済みを確認

## 2026-04-07: パフォーマンス最適化（機能変更なし）
- **A-1** SplitArchivePath の I/O 除去 (archive.cpp): GetFileAttributesW を削除し、拡張子チェック（IsArchiveFile）による純粋な文字列操作のみに変更。ファイルシステムアクセスを完全排除
- **A-2** StreamCacheGet のゼロコピー化 (stream_cache.cpp/h): 戻り値を `std::shared_ptr<const std::vector<BYTE>>` に変更。内部キャッシュも shared_ptr で保持し、呼び出し元（nav.cpp, prefetch.cpp, viewer.cpp, hover_preview.cpp）でフルコピーを排除
- **A-3** CacheGet/CacheContains のロック最適化 (cache.cpp/h): shared_mutex → mutex に変更（shared_lock 未使用のためオーバーヘッド削減）。CacheContains 関数追加（LRU更新なし、存在チェックのみ）。prefetch.cpp のキャッシュ存在チェックを CacheContains に置換
- **A-4** 設定キャッシュ導入 (settings.cpp/h): GetCachedSettings() で設定をメモリキャッシュ。SaveSettings 時に自動無効化。nav.cpp の ResolveIndex、prefetch.cpp の PrefetchStart、NavResetSettings から InvalidateSettingsCache を呼び出し

## 2026-04-07: コード品質改善（機能変更なし）
- **B-1** ToLowerW 関数化 (utils.h): `for (auto& c : key) c = towlower(c);` パターンを `ToLowerW()` に統合。filelist.cpp(6箇所), nav.cpp(5箇所), window.cpp(2箇所) を置換
- **B-2** SetMainTitle 関数化 (window.cpp/h): `L"karikari - " + path` + `SetWindowTextW` パターンを `SetMainTitle()` に統合。nav.cpp の3箇所を置換
- **B-3** IsViewableFile 統合 (filelist.cpp/h): `IsImageFile || IsArchiveFile || IsMediaFile` パターンを `IsViewableFile()` に統合。filelist.cpp の `IsSupportedFile` で使用
- **B-4** デッドコード削除: `ListArchiveEntries` (archive.cpp/h)、`PrefetchInit` (prefetch.cpp/h) を削除。プロジェクト全体で呼び出しなしを確認済み
- **B-5** g_sizeCache のLRU改善 (viewer.cpp): 500超過で全クリア → 1000超過で先頭500エントリを削除に変更。キャッシュの有効活用度向上
- **B-6** タイトルバー文字列構築最適化 (nav.cpp): SyncListViewAndStatus 内の std::wstring 連結を swprintf_s に置換。ヒープアロケーション削減

## 2026-04-07: D2D 1.0 → D2D 1.1 移行 + 画像デコード最適化
- **D2D 1.1 移行**: ID2D1HwndRenderTarget → ID2D1DeviceContext + IDXGISwapChain1
  - app.h: ViewerState を D2D 1.1 構造に変更（d2dFactory→ID2D1Factory1, deviceContext, d3dDevice, swapChain, backBuffer）
  - decoder.cpp: InitD2D() を D3D11 + DXGI + D2D 1.1 DeviceContext ベースに完全書き換え。WARPフォールバック付き
  - viewer.cpp: CreateViewerRenderTarget() をスワップチェーンベースに変更。WM_SIZE でスワップチェーンリサイズ。EndDraw後にPresent追加。デバイスロスト時にスワップチェーンも再作成
  - CMakeLists.txt: d3d11, dxgi をリンクライブラリに追加
  - 全ファイル（viewer.cpp, nav.cpp, window.cpp）の renderTarget 参照を deviceContext に置換
- **画像デコード最適化**:
  - DecodeMemoryToWicBitmap にマジックバイト判定追加（JPEG/PNG/WebP）→ コーデック検索スキップ
  - cache.cpp: EstimateBitmapSize を GetSize() → GetPixelSize() に変更（DPI非依存の正確なサイズ計算）
- **プリフェッチ修正**:
  - PrefetchCancel: WaitForThreadpoolWorkCallbacks(TRUE) → FALSE に変更（g_cancelFlagによる自然キャンセルで安全）
- **設計判断**:
  - ID2D1DeviceContext は ID2D1RenderTarget を継承するため、既存の描画コード（DrawBitmap, CreateBitmapFromWicBitmap等）は変更不要
  - D2D 1.1 DeviceContext経由でWICデコードすることで、内部的にYCbCr最適化パスが有効化される
  - ID2D1ImageSourceFromWic の本格利用は描画パス変更が大きいため見送り、DeviceContext移行による暗黙的最適化に任せる

## 2026-04-07: 3件の最適化（非同期デコード安全性・VSync・サムネイル並列度）
- **WM_ASYNC_DECODE_DONE をパスベースに変更** (viewer.h/cpp, window.cpp):
  - AsyncDecodeDoneMsg 構造体を導入し、ワーカーからパス・世代番号・WICBitmapをまとめて PostMessage で送信
  - 旧実装では g_app.nav.currentPath をキャッシュキーに使っていたため、高速ページ送り時にパスと画像が不整合になるリスクがあった
  - 構造体でパスを渡すことで、キャッシュキーとcurrentPathの両方が正しいデータに基づくようになった
- **Present のVSync条件分岐** (viewer.cpp):
  - 静止画表示時は Present(0,0) でVSyncなし即時表示、アニメーション時のみ Present(1,0)
  - 静止画切替の体感レイテンシ削減（1フレーム分≒16ms の待ち時間を排除）
- **サムネイルバッチサイズ拡大** (filelist.cpp):
  - kBatchSize を 4 → 12 に拡大。スレッドプールの並列度を活用してサムネイル生成を高速化

## 2026-04-07: 書庫オープン処理の非同期化
- **変更内容**: NavigateTo の IsArchiveFile 分岐で OpenArchiveAndGetEntries をバックグラウンドスレッド（スレッドプール）で実行するように変更
- **修正ファイル**: nav.cpp, nav.h, app.h, window.cpp
- **設計判断**:
  - 世代番号（g_archiveLoadGeneration）で古い結果を破棄。連続して書庫を切り替えた場合の競合を防止
  - UI即時更新（「読み込み中...」表示）で体感速度向上
  - 完了後は WM_ARCHIVE_LOADED カスタムメッセージでUIスレッドに結果を返却
  - SplitArchivePath 分岐（書庫内パス指定で開く場合）は同期のまま残した。既に書庫が開かれているケースが多く高速なため
  - ArchiveLoadResult 構造体を nav.h に定義（window.cpp で delete するため完全な型定義が必要）

## 2026-04-07: archive.cpp 3件の最適化
- **#1** MemoryOutStream の reserve 追加: ExtractToMemory / ExtractBatchToMemory で展開先バッファに entryList の既知サイズで事前 reserve。再アロケーション削減
- **#2** CreateFileW に FILE_FLAG_SEQUENTIAL_SCAN 追加: GetCachedArchive の tryOpen ラムダと IsRar5File の両方。OSのリードアヘッド最適化を有効化
- **#3** エントリディスクキャッシュを複数書庫対応: GetEntryCachePath が書庫パスのハッシュ値でファイル名を決定（arc_cache_{hash}.dat）。従来は archive_entries.cache 1ファイルに最後の1書庫分のみだったため、複数書庫を切り替えるとキャッシュミスが多発していた
