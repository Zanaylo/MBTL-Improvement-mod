#pragma once

#include <cstdint>

struct IDirect3DTexture9;

namespace PaletteTexture
{
	constexpr int kRows = 8;
	constexpr int kRowBytes = 1024;
	constexpr int kBytes = kRows * kRowBytes;

	IDirect3DTexture9* Own(uintptr_t owner);
	IDirect3DTexture9* Shared(uintptr_t shared);

	bool Read(IDirect3DTexture9* texture, uint8_t* outRgba);
	bool Write(IDirect3DTexture9* texture, const uint8_t* rgba, uint32_t rowMask);
}
