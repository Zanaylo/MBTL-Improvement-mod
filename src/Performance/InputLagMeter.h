#pragma once

namespace InputLagMeter
{
	bool Initialize();
	bool IsAvailable();

	void KeepAlive(int team);
	void Reset();

	float GetLastMs();
	float GetAverageMs();
	int GetTrustedCount();
}
