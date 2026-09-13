#pragma once

#include <cstdint>

namespace EffectPaint
{
	constexpr int kSeats = 4;
	constexpr int kColours = 256;
	constexpr int kBlockBytes = kColours * 4;

	bool GetObserved(int player, int entry, uint8_t* outRgb);

	void SetEntry(int player, int entry, const uint8_t* rgb);
	void ClearEntry(int player, int entry);
	void Clear(int player);

	void GetBlock(int player, uint8_t* block);
	void SetBlock(int player, const uint8_t* block);

	void SetRemote(int player, const uint8_t* block);
	void ClearRemote(int player);
	bool HasRemote(int player);
	bool GetRemoteEntry(int player, int entry, uint8_t* outRgb);

	unsigned GetRevision(int player);

	bool IsEdited(int player, int entry);
	bool GetEdit(int player, int entry, uint8_t* outRgb);
	int GetEditedCount(int player);

	void PreviewObserved(int player, const uint8_t* rgb, int except, const uint8_t* exceptRgb);
	void EndPreview(int player);

	void OnFrame();
}
