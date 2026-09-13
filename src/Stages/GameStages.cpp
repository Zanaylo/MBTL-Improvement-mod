#include "Stages/GameStages.h"

#include <windows.h>

#include <cstdio>

namespace {

constexpr int kRandomStage = 0;
constexpr int kShippedLast = 35;
constexpr int kShippedTraining = 90;
constexpr int kShippedDebug = 99;
constexpr int kShippedListed = 30;
constexpr int kShippedCard = 1;
constexpr int kShippedTracks = 35;

SRWLOCK g_lock = SRWLOCK_INIT;
std::vector<GameStages::Own> g_own;
std::vector<GameStages::Track> g_tracks;
int g_listed = kShippedListed;
int g_card = kShippedCard;
bool g_learned = false;
bool g_tracksLearned = false;

bool ShippedOwns(int number)
{
	return (number >= 1 && number <= kShippedLast) || number == kShippedTraining || number == kShippedDebug;
}

void ShippedTracks(std::vector<GameStages::Track>& out)
{
	out.clear();

	for (int id = 1; id <= kShippedTracks; ++id)
	{
		char file[24] = {};
		sprintf_s(file, "BGM %03d", id);
		out.push_back({ id, file });
	}
}

}

void GameStages::Learn(const std::vector<Own>& own, int listed, int templateCard)
{
	AcquireSRWLockExclusive(&g_lock);

	g_own = own;
	g_listed = listed;
	g_card = templateCard;
	g_learned = true;

	ReleaseSRWLockExclusive(&g_lock);
}

void GameStages::LearnTracks(const std::vector<Track>& tracks)
{
	AcquireSRWLockExclusive(&g_lock);

	g_tracks = tracks;
	g_tracksLearned = true;

	ReleaseSRWLockExclusive(&g_lock);
}

bool GameStages::Learned()
{
	AcquireSRWLockShared(&g_lock);
	const bool learned = g_learned;
	ReleaseSRWLockShared(&g_lock);

	return learned;
}

bool GameStages::TracksLearned()
{
	AcquireSRWLockShared(&g_lock);
	const bool learned = g_tracksLearned;
	ReleaseSRWLockShared(&g_lock);

	return learned;
}

bool GameStages::Owns(int number)
{
	AcquireSRWLockShared(&g_lock);

	bool owns = !g_learned && ShippedOwns(number);

	for (const Own& own : g_own)
		owns = owns || own.number == number;

	ReleaseSRWLockShared(&g_lock);

	return owns;
}

bool GameStages::Hidden(const Own& own)
{
	return own.number != kRandomStage && (own.selectDisabled || !own.listed);
}

void GameStages::Snapshot(std::vector<Own>& out)
{
	AcquireSRWLockShared(&g_lock);
	out = g_own;
	ReleaseSRWLockShared(&g_lock);
}

void GameStages::HiddenSnapshot(std::vector<Own>& out)
{
	out.clear();

	AcquireSRWLockShared(&g_lock);

	for (const Own& own : g_own)
	{
		if (Hidden(own))
			out.push_back(own);
	}

	ReleaseSRWLockShared(&g_lock);
}

void GameStages::TrackSnapshot(std::vector<Track>& out)
{
	AcquireSRWLockShared(&g_lock);

	if (g_tracksLearned)
		out = g_tracks;
	else
		ShippedTracks(out);

	ReleaseSRWLockShared(&g_lock);
}

int GameStages::ListedCount()
{
	AcquireSRWLockShared(&g_lock);
	const int listed = g_listed;
	ReleaseSRWLockShared(&g_lock);

	return listed;
}

int GameStages::TemplateCard()
{
	AcquireSRWLockShared(&g_lock);
	const int card = g_card;
	ReleaseSRWLockShared(&g_lock);

	return card;
}
