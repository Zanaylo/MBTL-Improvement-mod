#pragma once

class ITickListener
{
public:
	virtual ~ITickListener() = default;

	virtual void OnBattleTick() = 0;
};
