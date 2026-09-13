#include "Training/FrameMeter.h"

#include "Core/interfaces.h"
#include "Core/logger.h"
#include "Game/GameOffsets.h"
#include "Training/Commands.h"
#include "Training/GameState.h"
#include "Training/PlayerState.h"
#include "Training/Players.h"
#include "Training/SuperFlash.h"

#include <cstring>

namespace {

namespace Meter = GameOffsets::Meter;

using FrameMeter::Frame;
using FrameMeter::State;
using FrameMeter::kCapacity;
using FrameMeter::kPlayers;

constexpr int kIdleFramesToEnd = 24;
constexpr int kIdlePadding = 3;
constexpr int kLandingSlackFrames = 2;

struct Tracked
{
	uintptr_t entity = 0;
	uint16_t pattern = 0;
	bool sawActive = false;
	bool sawShield = false;
	bool airAttackLanding = false;
	bool inMove = false;
	uint32_t lastMvCountFrame = 0;
	bool hasMvCountFrame = false;
	bool wasAirborne = false;
	int landingSlack = 0;
	uint16_t stunPattern = 0;
	bool hasStunPattern = false;
	bool stunReleased = false;
	bool canAct = false;
	bool heldRun = false;
	bool heldPending = false;
	State heldLabel = State::None;
	State airKind = State::None;
	uint32_t lastCommand = 0;
};

struct Exchange
{
	bool hadAttack = false;
	int freeFrom = -1;
	int freeFromFrame = -1;
	uint16_t freePattern = 0;
	int moveEndAt = -1;
	int moveEndFrame = -1;
	int returnedAt = -1;
	int returnedFrame = -1;
	bool techSeen = false;
	int techStartedAt = -1;
	int techEndedFrame = -1;
};

struct MoveSpan
{
	bool valid = false;
	int startup = 0;
	int active = 0;
	int recovery = 0;
	int total = 0;
};

Frame g_frames[kPlayers][kCapacity] = {};
int g_length = 0;
int g_frame = 0;
int g_idleRun = 0;
bool g_recording = false;
int g_trackedCount = 0;

SuperFlash g_flash;
Tracked g_tracked[kPlayers];
Exchange g_exchange[kPlayers];
MoveSpan g_lastMove[kPlayers];

uint32_t g_lastBattleFrame = 0;
bool g_hasBattleFrame = false;

bool g_hasAdvantage = false;
int g_advantage = 0;
bool g_hasAfterRecovery = false;
int g_advantageAfterRecovery = 0;

bool ValidPlayer(int player)
{
	return player >= 0 && player < kPlayers;
}

void ResetMeter()
{
	g_length = 0;
	g_frame = 0;
	g_idleRun = 0;
	g_recording = false;
	g_trackedCount = 0;
	g_flash.Reset();

	g_hasBattleFrame = false;
	g_hasAdvantage = false;
	g_advantage = 0;
	g_hasAfterRecovery = false;
	g_advantageAfterRecovery = 0;

	for (int player = 0; player < kPlayers; ++player)
	{
		g_tracked[player] = Tracked();
		g_exchange[player] = Exchange();
		g_lastMove[player] = MoveSpan();
	}
}

bool BattleFrameAdvanced()
{
	const uint32_t counter = GameState::FrameCounter();

	if (g_hasBattleFrame && counter == g_lastBattleFrame)
		return false;

	const bool restarted = g_hasBattleFrame && counter < g_lastBattleFrame;

	if (restarted)
		ResetMeter();

	g_hasBattleFrame = true;
	g_lastBattleFrame = counter;
	return !restarted;
}

int RefreshEntities()
{
	int found = 0;

	for (int team = 0; team < kPlayers; ++team)
	{
		const uintptr_t chara = Players::Chara(team);
		found += chara != 0 ? 1 : 0;

		if (g_tracked[team].entity == chara)
			continue;

		g_tracked[team] = Tracked();
		g_tracked[team].entity = chara;
		LOG("FrameMeter: player %d is the character at 0x%08X", team + 1, static_cast<unsigned>(chara));
	}

	return found;
}

State StateOfKind(Commands::Kind kind)
{
	switch (kind)
	{
	case Commands::Kind_Dash:
		return State::Movement;
	case Commands::Kind_AirMovement:
		return State::AirMovement;
	case Commands::Kind_Jump:
		return State::Jump;
	default:
		return State::None;
	}
}

void UpdateCanAct(Tracked& tracked, const PlayerState::State& state)
{
	if (state.stunTimer > 0)
	{
		tracked.stunPattern = state.pattern;
		tracked.hasStunPattern = true;
		tracked.stunReleased = false;
	}
	else if (tracked.hasStunPattern && state.pattern == tracked.stunPattern)
	{
		tracked.stunReleased = true;
	}
	else
	{
		tracked.hasStunPattern = false;
		tracked.stunReleased = false;
	}

	tracked.canAct = state.actionable || (tracked.stunReleased && state.hitstop == 0);
}

void NoteNewMove(Tracked& tracked, const PlayerState::State& state)
{
	const bool airborne = PlayerState::IsAirborne(state);

	if (state.pattern != tracked.pattern)
	{
		tracked.pattern = state.pattern;
		tracked.sawActive = false;
		tracked.sawShield = false;
	}

	if (tracked.wasAirborne && !airborne)
		tracked.landingSlack = kLandingSlackFrames;
	else if (tracked.landingSlack > 0)
		--tracked.landingSlack;

	tracked.wasAirborne = airborne;

	const bool newMove = tracked.hasMvCountFrame && tracked.landingSlack == 0 &&
		state.mvCountFrame < tracked.lastMvCountFrame;

	tracked.lastMvCountFrame = state.mvCountFrame;
	tracked.hasMvCountFrame = true;

	if (!newMove)
		return;

	tracked.sawActive = false;
	tracked.sawShield = false;
	tracked.airAttackLanding = false;
	tracked.inMove = false;
}

State Movement(Tracked& tracked, const PlayerState::State& state)
{
	const bool airborne = PlayerState::IsAirborne(state);

	if ((state.moveCode[1] & Meter::Codes::kJump) != 0 && !tracked.sawActive && !tracked.airAttackLanding)
	{
		tracked.airKind = State::Jump;
		return State::Jump;
	}

	if (PlayerState::IsAttacking(state) || tracked.sawActive || tracked.airAttackLanding)
		return State::None;

	if ((state.moveCode[2] & Meter::Codes::kFromAssault) != 0)
		return State::None;

	const State kind = StateOfKind(Commands::KindOf(state.command));

	if (kind != State::None && !tracked.canAct)
	{
		if (airborne)
			tracked.airKind = kind;

		return kind;
	}

	if (state.running)
		return State::Movement;

	if (tracked.airKind != State::None && airborne && !tracked.canAct)
		return tracked.airKind;

	return State::None;
}

bool ContinuesAMove(int player)
{
	if (g_length <= 0)
		return false;

	const State previous = g_frames[player][g_length - 1].state;
	return previous == State::Active || previous == State::Recovery || previous == State::Cancellable;
}

State Recovering(const PlayerState::State& state)
{
	return PlayerState::CanLeaveFrameOnWhiff(state) ? State::Cancellable : State::Recovery;
}

State HeldState(Tracked& tracked, const PlayerState::State& state)
{
	if (!tracked.heldRun)
	{
		tracked.heldRun = true;
		tracked.heldLabel = state.inGuard ? State::Blockstun : State::Hitstun;
	}

	return tracked.heldLabel;
}

State ClassifyFree(int player, Tracked& tracked, const PlayerState::State& state)
{
	const State movement = Movement(tracked, state);

	if (movement != State::None)
		return movement;

	if (tracked.canAct && !state.actionable && tracked.inMove)
		return Recovering(state);

	const bool attacking = PlayerState::IsAttacking(state);
	const bool startingOwnMove = attacking && !tracked.sawActive && !tracked.inMove;

	if (tracked.canAct && !startingOwnMove)
		return State::Idle;

	if (!attacking && tracked.heldPending)
		return State::Hitstun;

	if (tracked.sawActive || tracked.sawShield || tracked.airAttackLanding)
		return Recovering(state);

	if (ContinuesAMove(player) && !attacking)
		return Recovering(state);

	return state.armor ? State::Armor : State::Startup;
}

State Classify(int player, const PlayerState::State& state)
{
	Tracked& tracked = g_tracked[player];
	const bool airborne = PlayerState::IsAirborne(state);

	UpdateCanAct(tracked, state);
	NoteNewMove(tracked, state);

	const bool held = !tracked.canAct && (state.stunTimer > 0 || state.inReaction || state.inGuard || tracked.stunReleased);
	const bool trulyFree = tracked.canAct && state.actionable;

	tracked.heldRun = held && tracked.heldRun;

	if (held || (tracked.canAct && !airborne))
		tracked.airKind = State::None;

	if (held)
		tracked.heldPending = true;

	if (tracked.canAct)
		tracked.heldPending = false;

	if (held || trulyFree)
		tracked.inMove = false;

	if (state.shield && !held)
	{
		tracked.sawShield = true;
		tracked.inMove = true;
		return State::Shield;
	}

	if (held || trulyFree)
		tracked.airAttackLanding = false;

	if (state.attackBoxes > 0)
	{
		tracked.sawActive = true;
		tracked.inMove = true;
		tracked.airAttackLanding = tracked.airAttackLanding || airborne;
		return State::Active;
	}

	return held ? HeldState(tracked, state) : ClassifyFree(player, tracked, state);
}

uint16_t DrawnInvuln(const PlayerState::State& state, State classified)
{
	uint16_t invuln = state.invuln;
	const bool held = classified == State::Hitstun || classified == State::Blockstun;

	if (PlayerState::IsAirborne(state) && !held && (invuln & PlayerState::Invuln_Full) == 0)
		invuln |= PlayerState::Invuln_Throw;

	if (held)
		invuln &= static_cast<uint16_t>(~PlayerState::Invuln_Throw);

	return invuln;
}

bool IsMoveCell(State state)
{
	return state == State::Startup || state == State::Armor || state == State::Active || state == State::Recovery ||
		state == State::Shield || state == State::Cancellable;
}

int LastMoveCell(int player)
{
	for (int i = g_length - 1; i >= 0; --i)
	{
		if (IsMoveCell(g_frames[player][i].state))
			return i;
	}

	return -1;
}

int MoveTail(int player, int begin, int end)
{
	const uint16_t movePattern = g_frames[player][begin].pattern;
	int last = end;

	while (last + 1 < g_length && g_frames[player][last + 1].state == State::Idle &&
		(g_frames[player][last + 1].pattern == movePattern || g_frames[player][last + 1].airborne))
	{
		++last;
	}

	return last;
}

void UpdateLastMoveFromBar(int player)
{
	const int end = LastMoveCell(player);

	if (end < 0)
		return;

	int begin = end;

	while (begin > 0 && IsMoveCell(g_frames[player][begin - 1].state))
		--begin;

	int active = 0;
	int startup = -1;
	int shieldAt = -1;

	for (int i = begin; i <= end; ++i)
	{
		const State state = g_frames[player][i].state;

		if (state == State::Shield && shieldAt < 0)
			shieldAt = i - begin + 1;

		if (state != State::Active)
			continue;

		startup = startup < 0 ? i - begin + 1 : startup;
		++active;
	}

	const int total = MoveTail(player, begin, end) - begin + 1;
	const int shownStartup = startup >= 0 ? startup : (shieldAt >= 0 ? shieldAt : total);

	MoveSpan& span = g_lastMove[player];
	span.valid = true;
	span.startup = shownStartup;
	span.active = active;
	span.recovery = total - shownStartup - active;
	span.total = total;
}

void Shift(int& index, int count)
{
	if (index < 0)
		return;

	index = index >= count ? index - count : -1;
}

void DropOldestFrames(int count)
{
	if (count <= 0 || count > g_length)
		return;

	const int remaining = g_length - count;

	for (int player = 0; player < kPlayers; ++player)
	{
		std::memmove(g_frames[player], g_frames[player] + count, sizeof(Frame) * static_cast<size_t>(remaining));

		Exchange& exchange = g_exchange[player];
		Shift(exchange.freeFrom, count);
		Shift(exchange.moveEndAt, count);
		Shift(exchange.returnedAt, count);
		Shift(exchange.techStartedAt, count);
	}

	g_length = remaining;
}

void BeginExchange()
{
	g_recording = true;
	g_length = 0;
	g_frame = 0;
	g_idleRun = 0;
	g_hasAdvantage = false;
	g_hasAfterRecovery = false;

	for (Exchange& exchange : g_exchange)
		exchange = Exchange();
}

void MarkFree(Exchange& exchange, const PlayerState::State& state)
{
	if (exchange.freeFrom >= 0)
		return;

	exchange.freeFrom = g_length;
	exchange.freeFromFrame = g_frame;
	exchange.freePattern = state.pattern;
	exchange.moveEndAt = -1;
	exchange.moveEndFrame = -1;
}

void ReviewFree(Exchange& exchange, const PlayerState::State& state, bool teching)
{
	if (state.stunTimer > 0 || teching || state.pattern != exchange.freePattern)
	{
		exchange.freeFrom = -1;
		exchange.freeFromFrame = -1;
		exchange.moveEndAt = -1;
		exchange.moveEndFrame = -1;
		return;
	}

	if (exchange.moveEndAt >= 0)
		return;

	exchange.moveEndAt = g_length;
	exchange.moveEndFrame = g_frame;
}

void UpdateExchange(int player, const PlayerState::State& state)
{
	Exchange& exchange = g_exchange[player];
	const bool teching = PlayerState::IsTeching(state);

	exchange.hadAttack = exchange.hadAttack || state.attackBoxes > 0 || state.projectileActive;

	if (g_tracked[player].canAct)
		MarkFree(exchange, state);
	else if (exchange.freeFrom >= 0)
		ReviewFree(exchange, state, teching);

	exchange.returnedAt = exchange.moveEndAt >= 0 ? exchange.moveEndAt : exchange.freeFrom;
	exchange.returnedFrame = exchange.moveEndFrame >= 0 ? exchange.moveEndFrame : exchange.freeFromFrame;

	if (!teching)
		return;

	if (!exchange.techSeen)
	{
		exchange.techSeen = true;
		exchange.techStartedAt = g_length;
	}

	exchange.techEndedFrame = g_frame + 1;
}

void ComputeAdvantage()
{
	const Exchange& first = g_exchange[0];
	const Exchange& second = g_exchange[1];

	if (g_trackedCount != kPlayers || (!first.hadAttack && !second.hadAttack))
		return;

	if (first.returnedFrame < 0 || second.returnedFrame < 0)
		return;

	g_advantage = second.returnedFrame - first.returnedFrame;
	g_hasAdvantage = true;

	if (!first.techSeen && !second.techSeen)
		return;

	const int firstEnd = first.techEndedFrame >= 0 ? first.techEndedFrame : first.returnedFrame;
	const int secondEnd = second.techEndedFrame >= 0 ? second.techEndedFrame : second.returnedFrame;
	const int afterTech = secondEnd - firstEnd;

	if (afterTech == g_advantage)
		return;

	g_advantageAfterRecovery = g_advantage;
	g_advantage = afterTech;
	g_hasAfterRecovery = true;
}

void FinishExchange()
{
	g_recording = false;
	g_hasAdvantage = false;
	ComputeAdvantage();

	if (g_settings.frameMeterTrace)
		LOG("meter: exchange ended, %d frames, advantage %s%d", g_length, g_hasAdvantage ? "" : "unknown ", g_advantage);
}

void WriteFrame(int player, const PlayerState::State* state, State classified)
{
	Frame& frame = g_frames[player][g_length];

	if (state == nullptr)
	{
		frame = Frame();
		return;
	}

	frame.state = classified;
	frame.projectileActive = state->projectileActive;
	frame.teching = PlayerState::IsTeching(*state);
	frame.airborne = PlayerState::IsAirborne(*state);
	frame.pattern = state->pattern;
	frame.invuln = DrawnInvuln(*state, classified);
}

void Trace(int player, const PlayerState::State& state, State classified, bool recorded)
{
	LOG("meter: f%d %s p%d %-12s pat=%u cmd=%u mc=%08X %08X %08X %08X %08X stance=%u free=%d cn=%u cs=%u stop=%d "
		"stun=%d react=%d guard=%d shield=%d armor=%d run=%d atk=%d prj=%d inv=%04X",
		g_frame, recorded ? "rec " : "skip", player + 1, FrameMeter::GetStateName(classified), state.pattern, state.command,
		state.moveCode[0], state.moveCode[1], state.moveCode[2], state.moveCode[3], state.moveCode[4], state.stance,
		state.actionable ? 1 : 0, state.cancelNormal, state.cancelSpecial, state.hitstop, state.stunTimer,
		state.inReaction ? 1 : 0, state.inGuard ? 1 : 0, state.shield ? 1 : 0, state.armor ? 1 : 0,
		state.running ? 1 : 0, state.attackBoxes, state.projectileActive ? 1 : 0, state.invuln);
}

void NoteCommand(int player, const PlayerState::State& state, State classified)
{
	Tracked& tracked = g_tracked[player];

	if (Commands::NameOf(state.command)[0] != '\0' && classified != State::Idle)
		tracked.lastCommand = state.command;
	else if (classified == State::Idle)
		tracked.lastCommand = 0;
}

bool ReadPlayers(PlayerState::State* states, bool* valid, int& live, int& frozen)
{
	live = 0;
	frozen = 0;

	for (int player = 0; player < kPlayers; ++player)
	{
		valid[player] = g_tracked[player].entity != 0 && PlayerState::Read(g_tracked[player].entity, states[player]);

		if (!valid[player])
			continue;

		++live;
		frozen += states[player].hitstop > 0 ? 1 : 0;
	}

	return live > 0;
}

bool ClassifyPlayers(const PlayerState::State* states, const bool* valid, State* classified)
{
	bool anyBusy = false;

	for (int player = 0; player < kPlayers; ++player)
	{
		if (!valid[player])
			continue;

		classified[player] = Classify(player, states[player]);
		NoteCommand(player, states[player], classified[player]);
		anyBusy = anyBusy || classified[player] != State::Idle || PlayerState::IsTeching(states[player]);
	}

	return anyBusy;
}

void Record(const PlayerState::State* states, const bool* valid, const State* classified, bool record)
{
	for (int player = 0; player < kPlayers; ++player)
	{
		if (record)
			WriteFrame(player, valid[player] ? &states[player] : nullptr, classified[player]);

		if (!valid[player])
			continue;

		if (g_settings.frameMeterTrace)
			Trace(player, states[player], classified[player], record);

		UpdateExchange(player, states[player]);
	}

	if (!record)
		return;

	++g_length;

	for (int player = 0; player < kPlayers; ++player)
	{
		if (valid[player])
			UpdateLastMoveFromBar(player);
	}
}

void Sample()
{
	if (!g_settings.frameMeterVisible || !GameState::AllowsTrainingTools())
	{
		if (g_length != 0 || g_recording)
			ResetMeter();

		return;
	}

	if (!BattleFrameAdvanced())
		return;

	g_trackedCount = RefreshEntities();

	PlayerState::State states[kPlayers] = {};
	bool valid[kPlayers] = {};
	State classified[kPlayers] = {};
	int live = 0;
	int frozen = 0;

	if (g_trackedCount == 0 || !ReadPlayers(states, valid, live, frozen))
		return;

	g_flash.Update(states, valid, kPlayers);

	const bool anyBusy = ClassifyPlayers(states, valid, classified);

	if (!g_recording && !anyBusy)
		return;

	if (!g_recording)
		BeginExchange();

	const bool inHitstop = frozen == live;
	const bool frozenTick = g_flash.IsRunning() ? g_flash.IsFreezing() : inHitstop;

	Record(states, valid, classified, !frozenTick && (anyBusy || g_idleRun < kIdlePadding));
	++g_frame;

	if (frozenTick)
		return;

	g_idleRun = anyBusy ? 0 : g_idleRun + 1;

	if (!anyBusy && g_idleRun >= kIdleFramesToEnd)
	{
		FinishExchange();
		return;
	}

	if (g_length >= kCapacity)
		DropOldestFrames(kCapacity / 4);
}

class Sampler final : public ITickListener
{
public:
	void OnBattleTick() override
	{
		Sample();
	}
};

Sampler g_sampler;

constexpr uint32_t Argb(uint32_t r, uint32_t g, uint32_t b)
{
	return (255u << 24) | (r << 16) | (g << 8) | b;
}

}

ITickListener* FrameMeter::Listener()
{
	return &g_sampler;
}

void FrameMeter::Reset()
{
	ResetMeter();
}

int FrameMeter::GetLength()
{
	return g_length;
}

bool FrameMeter::GetFrame(int player, int index, Frame& out)
{
	if (!ValidPlayer(player) || index < 0 || index >= g_length)
		return false;

	out = g_frames[player][index];
	return true;
}

bool FrameMeter::GetAdvantageFor(int player, int& outAdvantage)
{
	if (!g_hasAdvantage || !ValidPlayer(player))
		return false;

	outAdvantage = player == 0 ? g_advantage : -g_advantage;
	return true;
}

bool FrameMeter::GetAdvantageAfterRecoveryFor(int player, int& outAdvantage)
{
	if (!g_hasAdvantage || !g_hasAfterRecovery || !ValidPlayer(player))
		return false;

	outAdvantage = player == 0 ? g_advantageAfterRecovery : -g_advantageAfterRecovery;
	return true;
}

bool FrameMeter::GetLastMove(int player, int& outStartup, int& outActive, int& outRecovery, int& outTotal)
{
	if (!ValidPlayer(player) || !g_lastMove[player].valid)
		return false;

	outStartup = g_lastMove[player].startup;
	outActive = g_lastMove[player].active;
	outRecovery = g_lastMove[player].recovery;
	outTotal = g_lastMove[player].total;
	return true;
}

const char* FrameMeter::GetActionName(int player)
{
	return ValidPlayer(player) ? Commands::NameOf(g_tracked[player].lastCommand) : "";
}

int FrameMeter::GetFlashFrames(int player)
{
	return g_flash.GetFrames(player);
}

int FrameMeter::GetTrailingRunLength(int player)
{
	if (!ValidPlayer(player))
		return 0;

	int run = 0;

	for (int i = g_length - 1; i >= 0; --i)
	{
		const State state = g_frames[player][i].state;

		if (state == State::Idle || state == State::None)
			break;

		++run;
	}

	return run;
}

const char* FrameMeter::GetStateName(State state)
{
	switch (state)
	{
	case State::Idle: return "Free";
	case State::Startup: return "Startup";
	case State::Armor: return "Armor";
	case State::Active: return "Active";
	case State::Recovery: return "Recovery";
	case State::Cancellable: return "Cancellable";
	case State::Blockstun: return "Blockstun";
	case State::Hitstun: return "Hitstun";
	case State::Shield: return "Shield";
	case State::Movement: return "Dash";
	case State::Jump: return "Jump";
	case State::AirMovement: return "Air Dash";
	default: return "";
	}
}

uint32_t FrameMeter::GetStateColor(State state)
{
	switch (state)
	{
	case State::Idle: return Argb(64, 72, 86);
	case State::Startup: return Argb(64, 182, 226);
	case State::Armor: return Argb(158, 92, 214);
	case State::Active: return Argb(214, 48, 52);
	case State::Recovery: return Argb(46, 84, 176);
	case State::Cancellable: return Argb(64, 72, 86);
	case State::Blockstun: return Argb(86, 176, 126);
	case State::Hitstun: return Argb(226, 132, 48);
	case State::Shield: return Argb(96, 58, 146);
	case State::Movement: return Argb(212, 184, 72);
	case State::Jump: return Argb(36, 132, 180);
	case State::AirMovement: return Argb(112, 124, 236);
	default: return Argb(20, 22, 28);
	}
}

const char* FrameMeter::GetMarkerName(Marker marker)
{
	switch (marker)
	{
	case Marker_Projectile: return "Projectile Active";
	case Marker_Tech: return "Teching";
	case Marker_FullInvuln: return "Full Invincibility";
	case Marker_StrikeInvuln: return "Strike Invincibility";
	case Marker_ThrowInvuln: return "Throw Invincibility";
	case Marker_ProjectileInvuln: return "Projectile Invincibility";
	case Marker_HeadInvuln: return "Head Invincibility";
	case Marker_BodyInvuln: return "Body Invincibility";
	case Marker_LegsInvuln: return "Legs Invincibility";
	case Marker_DiveInvuln: return "Air Dive Invincibility";
	case Marker_HighMidInvuln: return "High and Mid Invincibility";
	case Marker_LowMidInvuln: return "Low and Mid Invincibility";
	default: return "";
	}
}

uint16_t FrameMeter::GetMarkerInvulnBit(Marker marker)
{
	switch (marker)
	{
	case Marker_FullInvuln: return PlayerState::Invuln_Full;
	case Marker_StrikeInvuln: return PlayerState::Invuln_Strike;
	case Marker_ThrowInvuln: return PlayerState::Invuln_Throw;
	case Marker_ProjectileInvuln: return PlayerState::Invuln_Projectile;
	case Marker_HeadInvuln: return PlayerState::Invuln_Head;
	case Marker_BodyInvuln: return PlayerState::Invuln_Body;
	case Marker_LegsInvuln: return PlayerState::Invuln_Legs;
	case Marker_DiveInvuln: return PlayerState::Invuln_Dive;
	case Marker_HighMidInvuln: return PlayerState::Invuln_HighMid;
	case Marker_LowMidInvuln: return PlayerState::Invuln_LowMid;
	default: return 0;
	}
}

uint32_t FrameMeter::GetMarkerColor(Marker marker)
{
	switch (marker)
	{
	case Marker_Projectile: return Argb(240, 170, 40);
	case Marker_Tech: return Argb(120, 240, 190);
	case Marker_FullInvuln: return Argb(255, 255, 255);
	case Marker_StrikeInvuln: return Argb(60, 140, 255);
	case Marker_ThrowInvuln: return Argb(255, 210, 74);
	case Marker_ProjectileInvuln: return Argb(0, 200, 170);
	case Marker_HeadInvuln: return Argb(255, 122, 47);
	case Marker_BodyInvuln: return Argb(124, 224, 74);
	case Marker_LegsInvuln: return Argb(180, 90, 255);
	case Marker_DiveInvuln: return Argb(255, 79, 208);
	case Marker_HighMidInvuln: return Argb(90, 200, 220);
	case Marker_LowMidInvuln: return Argb(220, 60, 60);
	default: return Argb(255, 255, 255);
	}
}
