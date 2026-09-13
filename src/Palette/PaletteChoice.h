#pragma once

namespace PaletteChoice
{
	constexpr int kSeats = 4;

	void OnFrame();

	const char* Remembered(int chara);
	void Remember(int chara, const char* file);
	void Forget(int chara);

	bool Apply(int seat, int chara, const char* file);

	bool Wear(int seat, const char* file);
	void Bare(int seat);

	bool Step(int seat, int steps);

	int LocalPlayer();

	const char* WornFile(int seat);
	void NoteWorn(int seat, const char* file);
	void NoteBare(int seat);

	unsigned GetGeneration(int seat);
}
