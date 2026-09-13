#include "D3D9/GameFont.h"

#include "Core/logger.h"
#include "D3D9/DdsTexture.h"
#include "D3D9/QuadRenderer.h"
#include "Game/GameAssets.h"

#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr uint8_t kMagic[] = { 'B', 'M', 'F', 3 };
constexpr size_t kBlockHeader = 5;
constexpr size_t kGlyphRecord = 20;
constexpr uint32_t kGlyphLimit = 128;
constexpr int kMaxPages = 16;
constexpr float kDefaultLineHeight = 30.0f;

enum Block : uint8_t
{
	Block_Common = 2,
	Block_Pages = 3,
	Block_Chars = 4,
};

struct Glyph
{
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	int16_t offsetX;
	int16_t offsetY;
	int16_t advance;
	uint8_t page;
	bool present;
};

struct Page
{
	std::string path;
	IDirect3DTexture9* texture;
	unsigned width;
	unsigned height;
	bool tried;
};

Glyph g_glyphs[kGlyphLimit] = {};
Page g_pages[kMaxPages] = {};
IDirect3DDevice9* g_device = nullptr;
float g_lineHeight = kDefaultLineHeight;
bool g_loaded = false;

void ParseCommon(const uint8_t* body, uint32_t length)
{
	uint16_t lineHeight = 0;

	if (length < sizeof(lineHeight))
		return;

	std::memcpy(&lineHeight, body, sizeof(lineHeight));
	g_lineHeight = static_cast<float>(lineHeight);
}

int ParsePages(const uint8_t* body, uint32_t length, const std::string& folder)
{
	const char* cursor = reinterpret_cast<const char*>(body);
	const char* const end = cursor + length;
	int count = 0;

	while (cursor < end && *cursor != '\0' && count < kMaxPages)
	{
		const size_t size = strnlen(cursor, static_cast<size_t>(end - cursor));
		g_pages[count++].path = folder + std::string(cursor, size);
		cursor += size + 1;
	}

	return count;
}

int ParseChars(const uint8_t* body, uint32_t length)
{
	int count = 0;

	for (uint32_t at = 0; at + kGlyphRecord <= length; at += kGlyphRecord)
	{
		const uint8_t* const record = body + at;
		uint32_t id = 0;

		std::memcpy(&id, record, sizeof(id));

		if (id >= kGlyphLimit)
			continue;

		Glyph& glyph = g_glyphs[id];
		std::memcpy(&glyph.x, record + 4, 2);
		std::memcpy(&glyph.y, record + 6, 2);
		std::memcpy(&glyph.width, record + 8, 2);
		std::memcpy(&glyph.height, record + 10, 2);
		std::memcpy(&glyph.offsetX, record + 12, 2);
		std::memcpy(&glyph.offsetY, record + 14, 2);
		std::memcpy(&glyph.advance, record + 16, 2);
		glyph.page = record[18];
		glyph.present = true;
		++count;
	}

	return count;
}

int ParseBlocks(const std::vector<uint8_t>& file, const std::string& folder)
{
	int glyphs = 0;
	size_t offset = sizeof(kMagic);

	while (offset + kBlockHeader <= file.size())
	{
		uint32_t length = 0;
		std::memcpy(&length, file.data() + offset + 1, sizeof(length));

		const size_t bodyAt = offset + kBlockHeader;

		if (bodyAt + length > file.size())
			break;

		const uint8_t* const body = file.data() + bodyAt;

		switch (file[offset])
		{
		case Block_Common:
			ParseCommon(body, length);
			break;
		case Block_Pages:
			ParsePages(body, length, folder);
			break;
		case Block_Chars:
			glyphs += ParseChars(body, length);
			break;
		default:
			break;
		}

		offset = bodyAt + length;
	}

	return glyphs;
}

std::string FolderOf(const char* path)
{
	const std::string text = path;
	const size_t slash = text.find_last_of("/\\");
	return slash == std::string::npos ? std::string() : text.substr(0, slash + 1);
}

bool IsFont(const std::vector<uint8_t>& file)
{
	return file.size() > sizeof(kMagic) && std::memcmp(file.data(), kMagic, sizeof(kMagic)) == 0;
}

Page* PageFor(uint8_t index)
{
	if (index >= kMaxPages || g_pages[index].path.empty())
		return nullptr;

	Page& page = g_pages[index];

	if (page.tried)
		return page.texture != nullptr ? &page : nullptr;

	page.tried = true;

	std::vector<uint8_t> data;

	if (!GameAssets::Read(page.path.c_str(), data))
	{
		LOG("GameFont: the page %s could not be read", page.path.c_str());
		return nullptr;
	}

	page.texture = DdsTexture::LoadFromMemory(g_device, data.data(), data.size(), page.path.c_str(), page.width,
		page.height);

	return page.texture != nullptr ? &page : nullptr;
}

}

bool GameFont::Load(IDirect3DDevice9* device, const char* fontPath)
{
	Release();

	std::vector<uint8_t> file;

	if (device == nullptr || !GameAssets::Read(fontPath, file))
	{
		LOG("GameFont: %s could not be read from the game's archives", fontPath);
		return false;
	}

	if (!IsFont(file))
	{
		LOG("GameFont: %s is not an AngelCode binary font", fontPath);
		return false;
	}

	g_device = device;

	const int glyphs = ParseBlocks(file, FolderOf(fontPath));
	g_loaded = glyphs > 0 && !g_pages[0].path.empty();

	LOG("GameFont: %s %s, %d glyphs, line height %.0f", fontPath, g_loaded ? "loaded" : "failed", glyphs, g_lineHeight);
	return g_loaded;
}

bool GameFont::IsLoaded()
{
	return g_loaded;
}

void GameFont::Release()
{
	for (Page& page : g_pages)
	{
		if (page.texture != nullptr)
			page.texture->Release();

		page = Page();
	}

	for (Glyph& glyph : g_glyphs)
		glyph = Glyph();

	g_device = nullptr;
	g_lineHeight = kDefaultLineHeight;
	g_loaded = false;
}

float GameFont::GetLineHeight()
{
	return g_lineHeight;
}

float GameFont::MeasureWidth(const char* text, float scale)
{
	if (text == nullptr || !g_loaded)
		return 0.0f;

	float width = 0.0f;

	for (const char* c = text; *c != '\0'; ++c)
	{
		const uint8_t id = static_cast<uint8_t>(*c);

		if (id < kGlyphLimit && g_glyphs[id].present)
			width += g_glyphs[id].advance * scale;
	}

	return width;
}

void GameFont::Draw(const char* text, float x, float y, float scale, uint32_t color)
{
	if (text == nullptr || !g_loaded)
		return;

	float penX = x;

	for (const char* c = text; *c != '\0'; ++c)
	{
		const uint8_t id = static_cast<uint8_t>(*c);

		if (id >= kGlyphLimit || !g_glyphs[id].present)
			continue;

		const Glyph& glyph = g_glyphs[id];
		const Page* const page = glyph.width > 0 && glyph.height > 0 ? PageFor(glyph.page) : nullptr;

		if (page != nullptr)
		{
			const float textureWidth = static_cast<float>(page->width);
			const float textureHeight = static_cast<float>(page->height);

			QuadRenderer::TexturedRect(page->texture, penX + glyph.offsetX * scale, y + glyph.offsetY * scale,
				glyph.width * scale, glyph.height * scale, glyph.x / textureWidth, glyph.y / textureHeight,
				(glyph.x + glyph.width) / textureWidth, (glyph.y + glyph.height) / textureHeight, color);
		}

		penX += glyph.advance * scale;
	}
}
