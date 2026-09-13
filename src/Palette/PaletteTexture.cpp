#include "Palette/PaletteTexture.h"

#include "Core/utils.h"
#include "Game/GameOffsets.h"

#include <d3d9.h>

namespace {

namespace Palette = GameOffsets::Palette;

constexpr int kEntries = 256;

IDirect3DTexture9* FromHolder(uint32_t holder)
{
	uint32_t engine = 0;
	uint32_t texture = 0;

	if (holder == 0 || !TryRead(holder, engine) || engine == 0)
		return nullptr;

	if (!TryRead(engine + Palette::kTextureInterface, texture) || texture == 0)
		return nullptr;

	return reinterpret_cast<IDirect3DTexture9*>(static_cast<uintptr_t>(texture));
}

bool Fits(IDirect3DTexture9* texture)
{
	D3DSURFACE_DESC desc = {};

	return SUCCEEDED(texture->GetLevelDesc(0, &desc)) && desc.Width == Palette::kTextureWidth &&
		desc.Height >= Palette::kTextureRows && desc.Format == D3DFMT_A8R8G8B8;
}

void CopyOut(const D3DLOCKED_RECT& locked, uint8_t* outRgba)
{
	for (int row = 0; row < PaletteTexture::kRows; ++row)
	{
		const uint8_t* const line = static_cast<const uint8_t*>(locked.pBits) + row * locked.Pitch;
		uint8_t* const out = outRgba + row * PaletteTexture::kRowBytes;

		for (int i = 0; i < kEntries; ++i)
		{
			out[i * 4 + 0] = line[i * 4 + 2];
			out[i * 4 + 1] = line[i * 4 + 1];
			out[i * 4 + 2] = line[i * 4 + 0];
			out[i * 4 + 3] = line[i * 4 + 3];
		}
	}
}

void CopyIn(const D3DLOCKED_RECT& locked, const uint8_t* rgba, uint32_t rowMask)
{
	for (int row = 0; row < PaletteTexture::kRows; ++row)
	{
		if ((rowMask & (1u << row)) == 0)
			continue;

		uint8_t* const line = static_cast<uint8_t*>(locked.pBits) + row * locked.Pitch;
		const uint8_t* const in = rgba + row * PaletteTexture::kRowBytes;

		for (int i = 0; i < kEntries; ++i)
		{
			line[i * 4 + 0] = in[i * 4 + 2];
			line[i * 4 + 1] = in[i * 4 + 1];
			line[i * 4 + 2] = in[i * 4 + 0];
			line[i * 4 + 3] = in[i * 4 + 3];
		}
	}
}

bool Transfer(IDirect3DTexture9* texture, uint8_t* read, const uint8_t* write, uint32_t rowMask)
{
	if (texture == nullptr)
		return false;

	__try
	{
		D3DLOCKED_RECT locked = {};

		if (!Fits(texture) || FAILED(texture->LockRect(0, &locked, nullptr, read != nullptr ? D3DLOCK_READONLY : 0)))
			return false;

		if (read != nullptr)
			CopyOut(locked, read);
		else
			CopyIn(locked, write, rowMask);

		texture->UnlockRect(0);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}

}

IDirect3DTexture9* PaletteTexture::Own(uintptr_t owner)
{
	uint32_t holder = 0;
	return owner != 0 && TryRead(owner + Palette::kOwnerTexture, holder) ? FromHolder(holder) : nullptr;
}

IDirect3DTexture9* PaletteTexture::Shared(uintptr_t shared)
{
	uint32_t holder = 0;
	return shared != 0 && TryRead(shared + Palette::kSharedTexture, holder) ? FromHolder(holder) : nullptr;
}

bool PaletteTexture::Read(IDirect3DTexture9* texture, uint8_t* outRgba)
{
	return outRgba != nullptr && Transfer(texture, outRgba, nullptr, 0);
}

bool PaletteTexture::Write(IDirect3DTexture9* texture, const uint8_t* rgba, uint32_t rowMask)
{
	return rgba != nullptr && rowMask != 0 && Transfer(texture, nullptr, rgba, rowMask);
}
