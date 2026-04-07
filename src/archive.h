#pragma once
#include "app.h"

bool InitArchive();       // 7z.dll ロード
void CloseArchive();      // クリーンアップ
void CloseCurrentArchive(); // キャッシュされた書庫ハンドルを閉じる
bool IsArchiveFile(const std::wstring& path);
// 書庫エントリ参照（所有権を持つ std::wstring を使用してダングリングポインタを防止）
struct ArchiveEntryRef { std::wstring path; ULONGLONG size; };
bool OpenArchiveAndGetEntries(const std::wstring& archivePath,
                              std::vector<ArchiveEntryRef>& outEntries);
bool ExtractToMemory(const std::wstring& archivePath,
                     const std::wstring& entryPath,
                     std::vector<BYTE>& buffer);

// バッチ展開: 1回のロックで複数エントリを展開（プリフェッチ用）
struct BatchExtractResult { std::wstring entryPath; std::vector<BYTE> buffer; bool ok; };
bool ExtractBatchToMemory(const std::wstring& archivePath,
                          const std::vector<std::wstring>& entryPaths,
                          std::vector<BatchExtractResult>& results);

// ハイブリッド展開: 小ファイル→メモリ、大ファイル→テンポラリ
// 戻り値: メモリの場合 buffer にデータ、テンポラリの場合 tempPath にファイルパス
bool ExtractSmart(const std::wstring& archivePath,
                  const std::wstring& entryPath,
                  ULONGLONG fileSize,
                  std::vector<BYTE>& buffer,
                  std::wstring& tempPath);

void CleanupTempFiles(); // テンポラリファイルの掃除

bool SplitArchivePath(const std::wstring& fullPath,
                      std::wstring& archivePath,
                      std::wstring& entryPath);
