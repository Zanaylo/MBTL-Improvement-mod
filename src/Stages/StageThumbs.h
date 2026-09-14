#pragma once

#include <vector>

namespace StageThumbs
{
	struct Card
	{
		int number;
		int card;
	};

	void Register();

	void Assign(std::vector<Card>& out);
	int CardIn(const std::vector<Card>& cards, int number);
	int LastCard();
}
