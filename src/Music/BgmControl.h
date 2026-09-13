#pragma once

#include <cstddef>
#include <cstdint>

namespace BgmControl
{
	bool Install(uint8_t* play, uint8_t* stop, uint8_t* start);
	bool IsHooked();
	const char* StatusText();
	bool RunsOnPresent();

	void Play(int id);
	void Stop();
	void Release();
	void Redraw();
	void RefreshVolume();

	void OnPresent();

	int HeldId();
	int LastAsked();
	int LastPlayed();
	long CallCount();
	void ReasonText(char* out, size_t size);

	bool HasPending();
	void PendingText(char* out, size_t size);

	unsigned long GameThread();
	unsigned long PresentThread();
}
