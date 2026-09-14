#include "D3D9/Post/UpscaleFilter.h"

#include <d3d9.h>

#include "D3D9/Post/Shaders/BicubicShader.h"
#include "D3D9/Post/Shaders/LanczosShader.h"
#include "D3D9/Post/Shaders/SceneUpscaleShader.h"

namespace {

struct Kernel
{
	const char* name;
	const char* description;
	const void* bytecode;
	bool linear;
};

const Kernel kKernels[UpscaleFilter::Kind_COUNT] = {
	{
		"Off",
		"The game's own basic stretch. No change.",
		nullptr,
		true,
	},
	{
		"Bicubic",
		"Sharper than the game's stretch at almost no cost, with no side effects. The safe choice.",
		kBicubicShader,
		true,
	},
	{
		"Lanczos",
		"The sharpest. Can add a faint halo where a bright line meets a dark one.",
		kLanczosShader,
		false,
	},
	{
		"FSR (EASU)",
		"AMD FSR upscaling. Follows edges, so diagonals come out as clean lines instead of steps. "
		"The best of these for hand drawn art.",
		kSceneUpscaleShader,
		false,
	},
};

}

int UpscaleFilter::Clamp(int kind)
{
	if (kind < Kind_Off)
		return Kind_Off;

	if (kind >= Kind_COUNT)
		return Kind_COUNT - 1;

	return kind;
}

const char* UpscaleFilter::GetName(int kind)
{
	return kKernels[Clamp(kind)].name;
}

const char* UpscaleFilter::Describe(int kind)
{
	return kKernels[Clamp(kind)].description;
}

const void* UpscaleFilter::GetBytecode(int kind)
{
	return kKernels[Clamp(kind)].bytecode;
}

bool UpscaleFilter::WantsLinear(int kind)
{
	return kKernels[Clamp(kind)].linear;
}
