#pragma once

#include <d3d9.h>

#include <cstdint>

namespace GameFont
{
	bool Load(IDirect3DDevice9* device, const char* fontPath);
	bool IsLoaded();
	void Release();

	float GetLineHeight();
	float MeasureWidth(const char* text, float scale);
	void Draw(const char* text, float x, float y, float scale, uint32_t color);
}
