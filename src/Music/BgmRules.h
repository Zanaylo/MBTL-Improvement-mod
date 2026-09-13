#pragma once

namespace BgmRules
{
	struct Rule
	{
		int from;
		int to;
		bool enabled;
	};

	void Load();

	int Count();
	const Rule& At(int index);

	bool Add(int from, int to);
	void Remove(int index);
	void SetEnabled(int index, bool enabled);
	void ForgetTrack(int id);

	int Resolve(int asked);
}
