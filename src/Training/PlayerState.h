#pragma once

#include <cstdint>

namespace PlayerState
{
	constexpr int kMoveCodes = 8;

	enum Invuln : uint16_t
	{
		Invuln_Full = 1u << 0,
		Invuln_Strike = 1u << 1,
		Invuln_Throw = 1u << 2,
		Invuln_Projectile = 1u << 3,
		Invuln_Head = 1u << 4,
		Invuln_Body = 1u << 5,
		Invuln_Legs = 1u << 6,
		Invuln_Dive = 1u << 7,
		Invuln_HighMid = 1u << 8,
		Invuln_LowMid = 1u << 9,
	};

	struct State
	{
		uint16_t pattern;
		uint32_t mvCountFrame;
		uint32_t command;
		uint32_t moveCode[kMoveCodes];

		int32_t hitstop;
		int32_t stunTimer;
		bool inReaction;
		bool inGuard;

		bool shield;
		bool armor;

		bool actionable;
		uint8_t cancelNormal;
		uint8_t cancelSpecial;

		uint8_t stance;
		bool running;

		int attackBoxes;
		bool projectileActive;
		uint16_t invuln;
	};

	bool Read(uintptr_t chara, State& out);

	bool IsAirborne(const State& state);
	bool IsAttacking(const State& state);
	bool IsTeching(const State& state);
	bool CanLeaveFrameOnWhiff(const State& state);
}
