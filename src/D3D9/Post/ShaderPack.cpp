#include "D3D9/Post/ShaderPack.h"

#include "Core/Settings.h"
#include "Core/info.h"
#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "D3D9/Post/BundledShaders.h"
#include "D3D9/Post/ShaderSource.h"

#include <d3dcompiler.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr int kMaxPacks = 64;
constexpr const char* kEntryPoint = "main";
constexpr const char* kTarget = "ps_3_0";
constexpr const char* kReadmeName = "README.txt";
constexpr const char* kInstallMarker = "installed.txt";
constexpr const char* kInstallStamp = MBTL_IM_VERSION " packs 1";

using D3DCompile_t = HRESULT(WINAPI*)(LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*,
	ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);

HMODULE g_compilerModule = nullptr;
D3DCompile_t g_compile = nullptr;
bool g_compilerTried = false;

std::vector<std::string> g_names;
std::string g_folder;
int g_selected = -1;

IDirect3DPixelShader9* g_shader = nullptr;
int g_compiled = -1;
bool g_compileFailed = false;

char g_status[512] = "no shader pack selected";
bool g_installed = false;

void Report(const char* format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(g_status, sizeof(g_status), format, arguments);
	va_end(arguments);
}

bool EnsureCompiler()
{
	if (g_compilerTried)
		return g_compile != nullptr;

	g_compilerTried = true;
	g_compilerModule = LoadLibraryA("d3dcompiler_47.dll");

	if (g_compilerModule == nullptr)
		g_compilerModule = LoadLibraryA("d3dcompiler_43.dll");

	if (g_compilerModule == nullptr)
	{
		LOG("[ShaderPack] no d3dcompiler on this system, user shaders are unavailable");
		return false;
	}

	g_compile = reinterpret_cast<D3DCompile_t>(GetProcAddress(g_compilerModule, "D3DCompile"));

	if (g_compile == nullptr)
		LOG("[ShaderPack] d3dcompiler loaded but has no D3DCompile export");

	return g_compile != nullptr;
}

void ReleaseShader()
{
	if (g_shader != nullptr)
		g_shader->Release();

	g_shader = nullptr;
	g_compiled = -1;
	g_compileFailed = false;
}

int IndexOf(const std::string& name)
{
	for (size_t i = 0; i < g_names.size(); ++i)
	{
		if (_stricmp(g_names[i].c_str(), name.c_str()) == 0)
			return static_cast<int>(i);
	}

	return -1;
}

bool HasExtension(const char* name, const char* extension)
{
	const size_t nameLength = strlen(name);
	const size_t extensionLength = strlen(extension);

	if (nameLength <= extensionLength)
		return false;

	return _stricmp(name + nameLength - extensionLength, extension) == 0;
}

void ScanExtension(const char* extension)
{
	WIN32_FIND_DATAA found = {};
	const HANDLE search = FindFirstFileA((g_folder + "*" + extension).c_str(), &found);

	if (search == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			continue;

		if (!HasExtension(found.cFileName, extension))
			continue;

		if (static_cast<int>(g_names.size()) >= kMaxPacks)
			break;

		g_names.push_back(found.cFileName);
	}
	while (FindNextFileA(search, &found) != 0);

	FindClose(search);
}

void Scan()
{
	g_names.clear();

	for (int i = 0; i < ShaderSource::ExtensionCount(); ++i)
		ScanExtension(ShaderSource::ExtensionAt(i));

	std::sort(g_names.begin(), g_names.end());
}

bool WriteBytes(const std::string& path, const void* data, size_t size);
void SaveTranslated(const std::string& name, const std::string& hlsl);

bool CompileSelected(IDirect3DDevice9* device)
{
	std::vector<uint8_t> source;
	const std::string& name = g_names[g_selected];
	const std::string path = g_folder + name;

	if (!ReadWholeFile(path, source, 1))
	{
		Report("%s could not be read", name.c_str());
		return false;
	}

	const ShaderSource::Format format = ShaderSource::DetectFormat(name.c_str());

	std::string hlsl;
	std::string note;

	if (!ShaderSource::Translate(format,
		std::string(reinterpret_cast<const char*>(source.data()), source.size()), hlsl, note))
	{
		Report("%s is a %s shader and %s", name.c_str(), ShaderSource::FormatName(format),
			note.c_str());

		LOG("[ShaderPack] %s", g_status);
		return false;
	}

	if (!ShaderSource::IsNative(format))
		SaveTranslated(name, hlsl);

	ID3DBlob* bytecode = nullptr;
	ID3DBlob* errors = nullptr;

	const HRESULT compiled = g_compile(hlsl.data(), hlsl.size(), path.c_str(), nullptr,
		nullptr, kEntryPoint, kTarget, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &bytecode, &errors);

	if (FAILED(compiled) || bytecode == nullptr)
	{
		Report("%s did not compile: %s%s", name.c_str(),
			errors != nullptr ? static_cast<const char*>(errors->GetBufferPointer())
			: "no message",
			ShaderSource::IsNative(format)
				? "" : ". Line numbers refer to the converted file in the Translated folder");

		LOG("[ShaderPack] %s", g_status);

		if (errors != nullptr)
			errors->Release();

		if (bytecode != nullptr)
			bytecode->Release();

		return false;
	}

	if (errors != nullptr)
		errors->Release();

	const HRESULT created = device->CreatePixelShader(
		static_cast<const DWORD*>(bytecode->GetBufferPointer()), &g_shader);

	bytecode->Release();

	if (FAILED(created))
	{
		g_shader = nullptr;
		Report("your GPU cannot run %s (needs pixel shader 3.0)", name.c_str());
		LOG("[ShaderPack] %s", g_status);
		return false;
	}

	g_compiled = g_selected;

	if (ShaderSource::IsNative(format))
		Report("%s compiled and running", name.c_str());
	else
		Report("%s compiled and running: %s", name.c_str(), note.c_str());
	LOG("[ShaderPack] %s", g_status);
	return true;
}

bool WriteBytes(const std::string& path, const void* data, size_t size)
{
	FILE* file = nullptr;

	if (fopen_s(&file, path.c_str(), "wb") != 0 || file == nullptr)
		return false;

	const size_t written = fwrite(data, 1, size, file);
	fclose(file);

	return written == size;
}

std::string TranslatedFolder()
{
	return g_folder + "Translated\\";
}

void SaveTranslated(const std::string& name, const std::string& hlsl)
{
	const std::string folder = TranslatedFolder();

	CreateDirectoryA(folder.c_str(), nullptr);
	WriteBytes(folder + name + ".hlsl", hlsl.data(), hlsl.size());
}

void WriteReadme()
{
	const char* const text =
		"MBTL Improvement Mod: shader packs\r\n"
		"\r\n"
		"A shader pack is one shader file in this folder. It runs on the final picture, after every other\r\n"
		"effect in the mod.\r\n"
		"\r\n"
		"\r\n"
		"HOW TO INSTALL A SHADER\r\n"
		"\r\n"
		"  1. Put the file in this folder (the one this README is in):\r\n"
		"\r\n"
		"         <the game folder>\\MBTL-IM\\Shaders\r\n"
		"\r\n"
		"     Not in a subfolder. The file name does not matter, only the extension.\r\n"
		"\r\n"
		"  2. Start the game, open the mod overlay (F1 by default), open Performance and go to the\r\n"
		"     Shaders tab.\r\n"
		"\r\n"
		"  3. Find \"Shader pack\" at the bottom of the tab. If the game was already running when you added\r\n"
		"     the file, press Rescan. Then pick your file in the list.\r\n"
		"\r\n"
		"  4. The file compiles as soon as you pick it. The line under the list shows the result, for\r\n"
		"     example:\r\n"
		"\r\n"
		"         crt-lottes.slang compiled and running: GLSL fragment stage, one pass\r\n"
		"\r\n"
		"     To turn it off, pick \"Off\" in the same list.\r\n"
		"\r\n"
		"  5. Your choice is saved in MBTL_IM.ini as [Graphics] ShaderPack, so it stays on next time.\r\n"
		"\r\n"
		"To remove a shader, delete its file. Nothing else is installed.\r\n"
		"\r\n"
		"\r\n"
		"FILE TYPES\r\n"
		"\r\n"
		"  .hlsl .ps      HLSL, compiled as it is\r\n"
		"  .fx            ReShade effect: HLSL with annotations and techniques\r\n"
		"  .slang         Vulkan GLSL with #pragma parameters\r\n"
		"  .glsl          OpenGL GLSL, new or old style\r\n"
		"  .frag .fsh     GLSL, same as .glsl\r\n"
		"\r\n"
		"Every type except .hlsl and .ps is CONVERTED to HLSL when you pick it. The converted file is saved as:\r\n"
		"\r\n"
		"    MBTL-IM\\Shaders\\Translated\\<the file name>.hlsl\r\n"
		"\r\n"
		"That is the file the compiler really uses. Open it when something goes wrong.\r\n"
		"\r\n"
		"\r\n"
		"LIMITS\r\n"
		"\r\n"
		"This game uses Direct3D 9. A shader must be HLSL compiled as pixel shader 3.0, and it gets ONE PASS\r\n"
		"over the final picture. Direct3D 9 cannot compile GLSL, so the mod converts other formats: settings\r\n"
		"become their default values, textures become the game picture, and size and time values come from\r\n"
		"the two constants listed below.\r\n"
		"\r\n"
		"Shaders that need a second pass, a lookup texture, the depth buffer or the previous frame will\r\n"
		"convert but look wrong. Most big CRT shaders use many passes and will not work. Single pass\r\n"
		"shaders do.\r\n"
		"\r\n"
		"\r\n"
		"WHEN A SHADER DOES NOT WORK\r\n"
		"\r\n"
		"The tab shows the compiler error. For a converted file, the line numbers refer to the converted copy:\r\n"
		"\r\n"
		"  1. Open MBTL-IM\\Shaders\\Translated\\<your file>.hlsl.\r\n"
		"  2. Go to the line in the error. The #define lines at the top show what the mod replaced each\r\n"
		"     setting with.\r\n"
		"  3. Fix it there, save it in the Shaders folder as your own .hlsl file, and pick that one instead.\r\n"
		"\r\n"
		"Common causes:\r\n"
		"\r\n"
		"  * the shader needs a second pass or a texture, and that is now a constant 0\r\n"
		"  * it uses a GLSL feature that HLSL does not have\r\n"
		"  * a #include the mod removed defined something the shader needs\r\n"
		"\r\n"
		"\r\n"
		"WHAT THE MOD PROVIDES\r\n"
		"\r\n"
		"  sampler2D Frame  : register(s0);   the picture so far\r\n"
		"  float4 FrameSize : register(c0);   xy = 1/width, 1/height   zw = width, height\r\n"
		"  float4 FrameTime : register(c1);   x  = seconds since load  y = frames since load\r\n"
		"\r\n"
		"Nothing else is provided: no vertex shader, no second pass, no previous frame and no depth buffer.\r\n"
		"uv goes from 0 to 1 across the window.\r\n"
		"\r\n"
		"\r\n"
		"WHAT A CONVERTED SHADER GETS\r\n"
		"\r\n"
		"  .fx        BUFFER_WIDTH, BUFFER_HEIGHT, BUFFER_RCP_WIDTH, BUFFER_RCP_HEIGHT, BUFFER_PIXEL_SIZE,\r\n"
		"             BUFFER_SCREEN_SIZE, BUFFER_ASPECT_RATIO, and the back buffer, pixel size and screen\r\n"
		"             size symbols. A uniform becomes its default value. A uniform with source timer or\r\n"
		"             framecount becomes FrameTime. Every sampler reads the picture, and the depth buffer\r\n"
		"             reads 1.0. The PixelShader of the first technique is the pass that runs\r\n"
		"  .slang     SourceSize, OriginalSize, OutputSize and FinalViewportSize as float4(w, h, 1/w, 1/h),\r\n"
		"             plus FrameCount, FrameDirection and MVP. Every #pragma parameter becomes its default.\r\n"
		"             Every sampler reads the picture, and the varying that carried the texture coordinate\r\n"
		"             becomes the coordinate of this pass. Only the fragment stage is kept\r\n"
		"  Shadertoy  iResolution, iTime, iTimeDelta, iFrame, iMouse, iDate and iChannel0 to iChannel3.\r\n"
		"             mainImage is called with fragCoord in pixels, y up\r\n"
		"  plain GLSL a sampler uniform becomes the picture, an in or varying becomes the coordinate, an out\r\n"
		"             or gl_FragColor becomes the result, and a uniform whose name looks like a resolution\r\n"
		"             or a time comes from FrameSize and FrameTime. Everything else becomes 0\r\n"
		"\r\n"
		"\r\n"
		"WRITING YOUR OWN\r\n"
		"\r\n"
		"The smallest shader that works, saved as anything.hlsl:\r\n"
		"\r\n"
		"  sampler2D Frame : register(s0);\r\n"
		"\r\n"
		"  float4 main(float2 uv : TEXCOORD0) : COLOR0\r\n"
		"  {\r\n"
		"      return float4(tex2D(Frame, uv).rgb, 1.0f);\r\n"
		"  }\r\n"
		"\r\n"
		"Entry point main, target ps_3_0, one pass, pixel shader only. Copy 01_passthrough.hlsl to start.\r\n"
		"\r\n"
		"Watch out: Frame uses POINT filtering, not linear. Reading at uv gives the exact pixels, which is\r\n"
		"right for pixel art. But a shader that moves the coordinates (curve, wobble, zoom) must filter by\r\n"
		"itself, or the picture shimmers. 12_crt.hlsl shows how, in SampleFrame.\r\n"
		"\r\n"
		"\r\n"
		"THE FILES IN THIS FOLDER\r\n"
		"\r\n"
		"  01_passthrough.hlsl       changes nothing. Copy it to start your own\r\n"
		"  02_grayscale.hlsl         black and white\r\n"
		"  03_sepia.hlsl             black and white with an old paper tint\r\n"
		"  04_invert.hlsl            inverted colours, like a photo negative\r\n"
		"  05_posterize.hlsl         fewer colours, with a 2x2 dither\r\n"
		"  06_pixelate.hlsl          big pixels\r\n"
		"  07_chromatic.hlsl         red and blue split apart towards the corners\r\n"
		"  08_film_grain.hlsl        moving film noise, mostly in dark areas\r\n"
		"  09_vhs.hlsl               VHS tape wobble, colour bleed and a noise band\r\n"
		"  10_lcd_grid.hlsl          the dot grid of a handheld screen\r\n"
		"  11_outline.hlsl           dark outlines on edges\r\n"
		"  12_crt.hlsl               CRT look: curve, scanlines, phosphor mask, bleed and vignette\r\n"
		"\r\n"
		"One example for each other file type, so you can compare it with its converted copy:\r\n"
		"\r\n"
		"  13_reshade_tonemap.fx     .fx: uniforms with annotations, a sampler and a technique\r\n"
		"  14_slang_scanlines.slang  .slang: #pragma parameters, a UBO, two stages\r\n"
		"  15_shadertoy_ripple.glsl  Shadertoy: one mainImage, iResolution and iTime\r\n"
		"  16_bleach_bypass.ps       HLSL with the .ps extension\r\n"
		"  17_dot_matrix.frag        new style GLSL: in, out, texture(), a sampler uniform\r\n"
		"  18_bloom_glow.fsh         old style GLSL: varying, gl_FragColor, texture2D, precision qualifiers\r\n"
		"\r\n"
		"The twelve .hlsl files keep their settings as #define lines at the top. Edit them, then pick the\r\n"
		"shader again on the tab to recompile it. The last six are there to compare with their converted\r\n"
		"copies in the Translated folder.\r\n"
		"\r\n"
		"The mod never overwrites files you add or edit. A file you delete stays deleted until the mod\r\n"
		"updates.\r\n"
		"\r\n"
		"\r\n"
		"IF NOTHING COMPILES\r\n"
		"\r\n"
		"Compiling needs d3dcompiler_47.dll, which comes with Windows and with Proton. Without it the tab\r\n"
		"says so and no shader is compiled. The rest of the tab still works.\r\n";

	WriteBytes(g_folder + kReadmeName, text, strlen(text));
}

bool AlreadyInstalled()
{
	std::vector<uint8_t> stamp;

	if (!ReadWholeFile(g_folder + kInstallMarker, stamp))
		return false;

	const std::string text(stamp.begin(), stamp.end());

	return text.compare(kInstallStamp) == 0;
}

void InstallBundled()
{
	if (g_installed)
		return;

	g_installed = true;

	if (AlreadyInstalled())
		return;

	WriteReadme();

	int written = 0;

	for (int i = 0; i < BundledShaders::Count(); ++i)
	{
		const uint8_t* data = nullptr;
		size_t size = 0;

		if (!BundledShaders::Get(i, data, size))
			continue;

		const std::string path = g_folder + BundledShaders::Name(i);

		if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES)
			continue;

		if (WriteBytes(path, data, size))
			++written;
	}

	WriteBytes(g_folder + kInstallMarker, kInstallStamp, strlen(kInstallStamp));

	LOG("[ShaderPack] wrote %d bundled example%s into %s", written, written == 1 ? "" : "s",
		g_folder.c_str());
}

}

void ShaderPack::Refresh()
{
	g_folder = GetModRootPath("Shaders") + "\\";
	InstallBundled();
	Scan();

	const int wanted = IndexOf(g_settings.shaderPack);

	if (wanted != g_selected)
		ReleaseShader();

	g_selected = wanted;

	if (g_names.empty())
	{
		Report("no shader files in %s", g_folder.c_str());
		return;
	}

	if (g_selected < 0)
	{
		Report("%d pack%s found, none selected", static_cast<int>(g_names.size()),
			g_names.size() == 1 ? "" : "s");
		return;
	}

	Report("%s selected", g_names[g_selected].c_str());
}

int ShaderPack::Count()
{
	return static_cast<int>(g_names.size());
}

const char* ShaderPack::GetName(int index)
{
	if (index < 0 || index >= Count())
		return "";

	return g_names[index].c_str();
}

int ShaderPack::GetSelected()
{
	return g_selected;
}

void ShaderPack::Select(int index)
{
	ReleaseShader();

	g_selected = index >= 0 && index < Count() ? index : -1;

	const char* const name = g_selected < 0 ? "" : g_names[g_selected].c_str();

	g_settings.shaderPack = name;
	Settings::SaveString("Graphics", "ShaderPack", name);

	Report(g_selected < 0 ? "no shader pack selected" : "%s selected", name);
}

IDirect3DPixelShader9* ShaderPack::Acquire(IDirect3DDevice9* device)
{
	if (device == nullptr || g_selected < 0 || g_compileFailed)
		return nullptr;

	if (g_shader != nullptr && g_compiled == g_selected)
		return g_shader;

	if (!EnsureCompiler())
	{
		Report("d3dcompiler_47.dll is missing, so shader packs cannot be compiled");
		g_compileFailed = true;
		return nullptr;
	}

	if (CompileSelected(device))
		return g_shader;

	g_compileFailed = true;
	return nullptr;
}

void ShaderPack::OnDeviceLost()
{
	ReleaseShader();
}

void ShaderPack::Shutdown()
{
	ReleaseShader();

	if (g_compilerModule == nullptr)
		return;

	FreeLibrary(g_compilerModule);
	g_compilerModule = nullptr;
	g_compile = nullptr;
	g_compilerTried = false;
}

bool ShaderPack::IsCompilerAvailable()
{
	return EnsureCompiler();
}

const char* ShaderPack::GetFolderPath()
{
	return g_folder.c_str();
}

const char* ShaderPack::GetStatusText()
{
	return g_status;
}
