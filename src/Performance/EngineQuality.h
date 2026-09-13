#pragma once

namespace EngineQuality
{
	bool IsAvailable();
	const char* LeverName();

	bool WantsStageEffects();
	bool ReadStageEffects(bool& outEnabled);

	void Apply();
	void OnFrame();

	const char* GetStatusText();
}
