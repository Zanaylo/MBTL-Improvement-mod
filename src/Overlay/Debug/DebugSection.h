#pragma once

#include <vector>

class IDebugSection
{
public:
	virtual ~IDebugSection() = default;

	virtual const char* Title() const = 0;
	virtual void Draw() = 0;
};

namespace DebugSections
{
	void Add(IDebugSection* section);
	const std::vector<IDebugSection*>& All();
}
