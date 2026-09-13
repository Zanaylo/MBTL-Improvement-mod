#pragma once

namespace Camera
{
	struct View
	{
		float x;
		float y;
		float zoom;
	};

	bool IsAvailable();

	bool Read(View& out);
	bool ReadElement(int element, View& out);

	void ToScreen(const View& view, float worldX, float worldY, float width, float height, float& outX, float& outY);

	int ElementCount();
	const char* ElementName(int element);
}
