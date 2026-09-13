#pragma once

#include <string>

namespace StageEntry
{
	std::string Compose(const std::string& templateBlock, const std::string& source, int number,
		const std::string& shiftJisName, int card);

	std::string ShiftJis(const std::string& utf8);
}
