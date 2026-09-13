#pragma once

#include "Game/FileOverlay.h"

class BgmTextOverlay : public IFileOverlay
{
public:
	bool Covers(const std::string& key) const override;
	bool Apply(const std::string& key, std::vector<uint8_t>& content) const override;
	uint32_t Version() const override;
};
