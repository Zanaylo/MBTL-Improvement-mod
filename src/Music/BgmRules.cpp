#include "Music/BgmRules.h"

#include "Core/logger.h"
#include "Music/BgmTable.h"
#include "Music/MusicIni.h"

#include <windows.h>

#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr const char* kSection = "Rules";
constexpr int kMostRules = 64;
constexpr int kRuleFields = 3;

BgmRules::Rule g_rules[kMostRules] = {};
int g_count = 0;
SRWLOCK g_lock = SRWLOCK_INIT;

void Save()
{
	std::vector<MusicIni::Entry> entries;

	for (int i = 0; i < g_count; ++i)
	{
		char value[48] = {};
		sprintf_s(value, "%d,%d,%d", g_rules[i].from, g_rules[i].to, g_rules[i].enabled ? 1 : 0);
		entries.emplace_back(std::to_string(i), value);
	}

	MusicIni::WriteSection(kSection, entries);
}

void EraseAt(int index)
{
	for (int i = index; i + 1 < g_count; ++i)
		g_rules[i] = g_rules[i + 1];

	--g_count;
}

bool IsValidIndex(int index)
{
	return index >= 0 && index < g_count;
}

}

void BgmRules::Load()
{
	g_count = 0;

	for (const MusicIni::Entry& entry : MusicIni::ReadSection(kSection))
	{
		int from = BgmTable::kNoTrack;
		int to = BgmTable::kNoTrack;
		int enabled = 0;

		if (sscanf_s(entry.second.c_str(), "%d,%d,%d", &from, &to, &enabled) != kRuleFields)
			continue;

		if (g_count >= kMostRules || from == to || !BgmTable::IsValidId(from) || !BgmTable::IsValidId(to))
			continue;

		g_rules[g_count++] = { from, to, enabled != 0 };
	}

	LOG("BgmRules: %d rule(s)", g_count);
}

int BgmRules::Count()
{
	return g_count;
}

const BgmRules::Rule& BgmRules::At(int index)
{
	return g_rules[index];
}

bool BgmRules::Add(int from, int to)
{
	if (g_count >= kMostRules || from == to || !BgmTable::IsValidId(from) || !BgmTable::IsValidId(to))
		return false;

	AcquireSRWLockExclusive(&g_lock);
	g_rules[g_count++] = { from, to, true };
	ReleaseSRWLockExclusive(&g_lock);

	Save();
	return true;
}

void BgmRules::Remove(int index)
{
	if (!IsValidIndex(index))
		return;

	AcquireSRWLockExclusive(&g_lock);
	EraseAt(index);
	ReleaseSRWLockExclusive(&g_lock);

	Save();
}

void BgmRules::SetEnabled(int index, bool enabled)
{
	if (!IsValidIndex(index))
		return;

	AcquireSRWLockExclusive(&g_lock);
	g_rules[index].enabled = enabled;
	ReleaseSRWLockExclusive(&g_lock);

	Save();
}

void BgmRules::ForgetTrack(int id)
{
	const int before = g_count;

	AcquireSRWLockExclusive(&g_lock);

	for (int i = g_count; i-- > 0;)
	{
		if (g_rules[i].from == id || g_rules[i].to == id)
			EraseAt(i);
	}

	ReleaseSRWLockExclusive(&g_lock);

	if (g_count == before)
		return;

	Save();
}

int BgmRules::Resolve(int asked)
{
	int chosen = BgmTable::kNoTrack;

	AcquireSRWLockShared(&g_lock);

	for (int i = 0; i < g_count && chosen == BgmTable::kNoTrack; ++i)
	{
		const Rule& rule = g_rules[i];

		if (rule.enabled && rule.from == asked && BgmTable::IsPresent(rule.to))
			chosen = rule.to;
	}

	ReleaseSRWLockShared(&g_lock);
	return chosen;
}
