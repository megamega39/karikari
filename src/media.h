#pragma once
#include "app.h"

bool IsVideoFile(const std::wstring& path);
bool IsAudioFile(const std::wstring& path);
bool IsMediaFile(const std::wstring& path);

bool MediaInit(HINSTANCE hInst);
void MediaShutdown();
bool RegisterMediaPlayerClass(HINSTANCE hInst);
HWND CreateMediaPlayer(HWND parent, HINSTANCE hInst);

void MediaPlay(const std::wstring& path);
void MediaStop();
void MediaTogglePlayPause();
void MediaSeek(double seconds);
void MediaSetVolume(double vol);   // 0.0-1.0
void MediaSetSpeed(double speed);  // 0.5-2.0
void MediaToggleLoop();
double MediaGetPosition();
double MediaGetDuration();
bool MediaIsPlaying();
