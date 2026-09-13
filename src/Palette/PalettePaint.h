#pragma once

#include <cstdint>

namespace PalettePaint
{
	constexpr int kSeats = 4;
	constexpr int kSubs = 4;
	constexpr int kColours = 256;
	constexpr int kBytes = kColours * 4;

	void Stage(int player, int sub, const uint8_t* colours);
	void Clear(int player);
	bool IsStaged(int player);
	const uint8_t* GetStaged(int player, int sub);

	void Preview(int player, int sub, const uint8_t* colours);
	void EndPreview(int player);

	void StageRemote(int player, int sub, const uint8_t* colours);
	void ClearRemote(int player);
	bool HasRemote(int player);
	const uint8_t* GetRemote(int player, int sub);

	void OnFrame();

	bool IsPainting(int player);
	bool ReadGameColours(int player, int sub, uint8_t* rgba);
	const uint8_t* GetWorn(int player, int sub);

	unsigned GetRevision(int player);
}
