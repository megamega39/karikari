#pragma once

// ナビゲーションインデックス解決（ループ設定を引数で受け取る純粋関数版）
inline int ResolveNavIndex(int index, int total, bool wrap)
{
    if (total <= 0) return -1;
    if (wrap)
    {
        if (index < 0) index = total - 1;
        if (index >= total) index = 0;
    }
    else
    {
        if (index < 0 || index >= total) return -1;
    }
    return index;
}
