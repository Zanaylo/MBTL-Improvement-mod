#pragma once

#include "Training/TickListener.h"

#include <cstdint>

namespace FrameMeter
{
	enum class State : uint8_t
	{
		None,
		Idle,
		Startup,
		Armor,
		Active,
		Recovery,
		Blockstun,
		Hitstun,
		Shield,
		Cancellable,
		Movement,
		Jump,
		AirMovement,
	};

	enum Marker
	{
		Marker_Projectile,
		Marker_Tech,

		Marker_FullInvuln,
		Marker_StrikeInvuln,
		Marker_ThrowInvuln,
		Marker_ProjectileInvuln,
		Marker_HeadInvuln,
		Marker_BodyInvuln,
		Marker_LegsInvuln,
		Marker_DiveInvuln,
		Marker_HighMidInvuln,
		Marker_LowMidInvuln,

		Marker_COUNT
	};

	constexpr int kPlayers = 2;
	constexpr int kCapacity = 600;
	constexpr int kFirstInvulnMarker = Marker_FullInvuln;

	struct Frame
	{
		State state;
		bool projectileActive;
		bool teching;
		uint16_t invuln;
		bool airborne;
		uint16_t pattern;
	};

	ITickListener* Listener();
	void Reset();

	int GetLength();
	bool GetFrame(int player, int index, Frame& out);

	bool GetAdvantageFor(int player, int& outAdvantage);
	bool GetAdvantageAfterRecoveryFor(int player, int& outAdvantage);
	bool GetLastMove(int player, int& outStartup, int& outActive, int& outRecovery, int& outTotal);
	const char* GetActionName(int player);
	int GetFlashFrames(int player);
	int GetTrailingRunLength(int player);

	const char* GetStateName(State state);
	uint32_t GetStateColor(State state);

	uint16_t GetMarkerInvulnBit(Marker marker);
	const char* GetMarkerName(Marker marker);
	uint32_t GetMarkerColor(Marker marker);
}
