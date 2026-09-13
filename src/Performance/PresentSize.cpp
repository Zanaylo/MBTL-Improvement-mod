#include "Performance/PresentSize.h"

#include "Core/interfaces.h"
#include "Performance/Improvements.h"
#include "Performance/PotatoMode.h"

void PresentSize::Refresh()
{
	int width = 0;
	int height = 0;

	if (!PotatoMode::GetPresentSize(width, height))
		Improvements::GetPresentSize(width, height);

	g_settings.presentWidth = width;
	g_settings.presentHeight = height;
}
