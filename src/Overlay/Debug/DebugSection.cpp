#include "Overlay/Debug/DebugSection.h"

namespace {

std::vector<IDebugSection*>& Registry()
{
	static std::vector<IDebugSection*> sections;
	return sections;
}

}

void DebugSections::Add(IDebugSection* section)
{
	if (section == nullptr)
		return;

	Registry().push_back(section);
}

const std::vector<IDebugSection*>& DebugSections::All()
{
	return Registry();
}
