#pragma once

namespace BgmShuffle
{
	void Load();

	bool IsEnabled();
	void SetEnabled(bool enabled);

	bool IsInDraw(int id, bool loops);
	void SetInDraw(int id, bool inDraw);
	void SetAllInDraw(bool inDraw);

	int Pick(int asked);
	void Redraw();
}
