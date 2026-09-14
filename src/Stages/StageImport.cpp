#include "Stages/StageImport.h"

#include "Core/TextEncoding.h"
#include "Core/logger.h"
#include "Core/utils.h"
#include "Game/ModFiles.h"
#include "Stages/BgObjectFix.h"
#include "Stages/FbGameFolder.h"
#include "Stages/GameStages.h"
#include "Stages/HiddenStages.h"
#include "Stages/StageArchive.h"
#include "Stages/StageLibrary.h"
#include "Stages/StageNames.h"
#include "Stages/StagePicker.h"
#include "Stages/StageRevision.h"

#include <windows.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

namespace {

using Entry = StageLibrary::Entry;

constexpr const char* kObjectList = "object.txt";
constexpr const char* kStageNote = "stage.txt";
constexpr const char* kModel = "bg.fbx.bin";
constexpr const char* kNodeList = "nodes.txt";
constexpr const char* kCustomGame = "Custom stage";
constexpr const char* kStageMark = "./bg/";
constexpr const char* kNoNumber = "No free stage number left. Remove a stage first.";
constexpr int kDfciPriorityFloor = 700;
constexpr long kWhole = 100;

const char* const kSkippedExtensions[] = { ".fbx", ".json" };

struct Job
{
	std::string stage;
	std::string name;
	std::string list;
	std::string path;
	int number;
};

struct Batch
{
	std::string folder;
	bool custom;
	bool replace;
	std::vector<Job> jobs;
};

std::vector<StageImport::Offer> g_offers;
std::string g_scanFolder;
std::string g_scanGame;

SRWLOCK g_statusLock = SRWLOCK_INIT;
char g_status[224] = "No game folder picked yet.";
char g_shown[224] = {};

volatile long g_busy = 0;
volatile long g_finished = 0;
volatile long g_progress = 0;

void SetStatus(const char* format, ...)
{
	char text[sizeof(g_status)] = {};

	va_list args;
	va_start(args, format);
	vsnprintf_s(text, sizeof(text), _TRUNCATE, format, args);
	va_end(args);

	AcquireSRWLockExclusive(&g_statusLock);
	memcpy(g_status, text, sizeof(g_status));
	ReleaseSRWLockExclusive(&g_statusLock);
}

bool EndsWith(const std::string& text, const char* tail)
{
	const size_t length = strlen(tail);

	return text.size() >= length && _stricmp(text.c_str() + text.size() - length, tail) == 0;
}

bool Skipped(const std::string& file)
{
	if (_stricmp(file.c_str(), kNodeList) == 0 || _stricmp(file.c_str(), kStageNote) == 0)
		return true;

	return std::any_of(std::begin(kSkippedExtensions), std::end(kSkippedExtensions),
		[&file](const char* tail) { return EndsWith(file, tail); });
}

std::string CleanName(const char* name)
{
	std::string out = name == nullptr ? std::string() : name;

	out.erase(std::remove_if(out.begin(), out.end(), [](char c) { return c == '"' || c == '|' || c == '\r' || c == '\n'; }),
		out.end());

	const size_t first = out.find_first_not_of(' ');
	const size_t last = out.find_last_not_of(' ');

	return first == std::string::npos ? std::string() : out.substr(first, last - first + 1);
}

std::string Leaf(const std::string& path)
{
	const size_t slash = path.find_last_of("\\/");

	return slash == std::string::npos ? path : path.substr(slash + 1);
}

bool WriteData(const std::string& path, const std::vector<uint8_t>& data)
{
	return WriteWholeFile(path, data.data(), data.size());
}

void Rename(int number, std::vector<uint8_t>& data)
{
	char now[24] = {};
	sprintf_s(now, "%sbg%03d/", kStageMark, number);

	const size_t mark = strlen(kStageMark);
	std::string text(data.begin(), data.end());

	for (size_t at = text.find(kStageMark); at != std::string::npos; at = text.find(kStageMark, at))
	{
		const size_t close = text.find('/', at + mark);

		if (close == std::string::npos)
			break;

		text.replace(at, close + 1 - at, now);
		at += strlen(now);
	}

	data.assign(text.begin(), text.end());
}

void TrimTable(std::vector<uint8_t>& data)
{
	const std::string text(data.begin(), data.end());
	const size_t table = text.find("<-");
	const size_t open = table == std::string::npos ? table : text.find('{', table);
	const size_t end = open == std::string::npos ? open : StageArchive::MatchPair(text, open);

	if (end != std::string::npos && end < data.size())
		data.resize(end);
}

void FixObjects(StageArchive::Source& source, const Job& job, FbGameFolder::Game game, std::vector<uint8_t>& data)
{
	const std::string sprites = BgObjectFix::SpriteFile(data);
	std::vector<uint8_t> pat;

	if (!sprites.empty())
		source.Read(job.stage, sprites, pat);

	const BgObjectFix::Report fixes = BgObjectFix::Apply(pat, data, game == FbGameFolder::Game_DFCI ? kDfciPriorityFloor : 0);

	if (fixes.sprites != 0 || fixes.raised != 0 || fixes.renumbered != 0)
		LOG("StageImport: %s object layer - %d sprite name(s) repaired, %d priority raised, %d block(s) renumbered",
			job.stage.c_str(), fixes.sprites, fixes.raised, fixes.renumbered);

	for (const std::string& name : fixes.lost)
		LOG("StageImport: %s draws sprite %s and %s holds no such pattern", kObjectList, name.c_str(), sprites.c_str());

	Rename(job.number, data);
	TrimTable(data);
}

void ReportProgress(long base, long share, size_t done, size_t total)
{
	InterlockedExchange(&g_progress, base + static_cast<long>(total == 0 ? 0 : (share * done) / total));
}

bool CopyFromSource(StageArchive::Source& source, Job& job, FbGameFolder::Game game, long base, long share)
{
	std::vector<std::string> files;
	source.Files(job.stage, files);

	if (files.empty())
	{
		SetStatus("%s has no files the mod can read.", job.stage.c_str());
		return false;
	}

	const std::string target = StageLibrary::FolderOf(job.number);
	CreateDirectoryTree(target);

	int written = 0;

	for (size_t i = 0; i < files.size(); ++i)
	{
		ReportProgress(base, share, i, files.size());

		const std::string& file = files[i];
		std::vector<uint8_t> data;

		if (Skipped(file))
			continue;

		if (!source.Read(job.stage, file, data) || !StageArchive::MagicOk(file, data))
		{
			LOG("StageImport: %s\\%s came out wrong and was left out", job.stage.c_str(), file.c_str());
			continue;
		}

		if (_stricmp(file.c_str(), kObjectList) == 0)
			FixObjects(source, job, game, data);

		written += WriteData(target + "\\" + file, data) ? 1 : 0;
	}

	if (written == 0)
	{
		SetStatus("No file in %s could be read.", job.stage.c_str());
		return false;
	}

	std::string list;
	source.BgList(list);
	StageArchive::Block(list, job.stage, job.list);
	return true;
}

bool CopyFolder(Job& job)
{
	const std::string target = StageLibrary::FolderOf(job.number);
	CreateDirectoryTree(target);

	WIN32_FIND_DATAA found = {};
	const HANDLE search = FindFirstFileA((job.path + "\\*").c_str(), &found);

	if (search == INVALID_HANDLE_VALUE)
		return false;

	int written = 0;

	do
	{
		const std::string file = found.cFileName;
		std::vector<uint8_t> data;

		if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 || !ReadWholeFile(job.path + "\\" + file, data))
			continue;

		if (_stricmp(file.c_str(), kStageNote) == 0)
			job.list.assign(data.begin(), data.end());

		if (Skipped(file))
			continue;

		if (_stricmp(file.c_str(), kObjectList) == 0)
			Rename(job.number, data);

		written += WriteData(target + "\\" + file, data) ? 1 : 0;
	}
	while (FindNextFileA(search, &found) != 0);

	FindClose(search);

	if (written != 0)
		return true;

	SetStatus("%s has no files the mod can read.", job.path.c_str());
	return false;
}

std::string NoteName(const std::string& note)
{
	std::string value;

	return StageArchive::Field(note, "Name", value) ? CleanName(StageArchive::Unquoted(value).c_str()) : std::string();
}

void WriteNote(const Job& job, const char* game)
{
	std::string note = "Name = \"" + job.name + "\"\r\n";
	note += std::string("From = \"") + game + "\"\r\n";
	note += "Source = \"" + job.stage + "\"\r\n\r\n";
	note += job.list;

	WriteWholeFile(StageLibrary::NoteOf(job.number), note);
}

void FillName(Job& job)
{
	if (job.name.empty())
		job.name = NoteName(job.list);

	if (job.name.empty())
		job.name = job.stage;
}

void Register(Job& job, const char* game)
{
	FillName(job);
	WriteNote(job, game);

	Entry entry = {};
	entry.number = job.number;
	entry.shown = StagePicker::Room() > 0;
	entry.card = StageLibrary::kTemplateCard;
	entry.music = StageLibrary::kDefaultMusic;
	entry.game = game;
	entry.folder = job.stage;
	entry.name = job.name;

	StageLibrary::Put(entry);

	LOG("StageImport: %s is stage %d, %s the picker", job.name.c_str(), job.number, entry.shown ? "in" : "not in");
}

void Replace(Job& job)
{
	FillName(job);
	WriteNote(job, kCustomGame);

	LOG("StageImport: %s replaces stage %d", job.name.c_str(), job.number);
}

int RunBatch(Batch& batch)
{
	const FbGameFolder::Game game = batch.custom ? FbGameFolder::Game_None : FbGameFolder::Detect(batch.folder.c_str());
	const std::unique_ptr<StageArchive::Source> source = batch.custom ? nullptr : StageArchive::Open(batch.folder.c_str());

	if (!batch.custom && source == nullptr)
	{
		SetStatus("Could not open that game's stage files.");
		return 0;
	}

	const long share = kWhole / static_cast<long>(batch.jobs.size());
	const char* const from = batch.custom ? kCustomGame : FbGameFolder::Name(game);
	int done = 0;

	for (size_t i = 0; i < batch.jobs.size(); ++i)
	{
		Job& job = batch.jobs[i];
		const DWORD began = GetTickCount();

		if (batch.replace)
			StageLibrary::DeleteFiles(job.number);

		const bool copied = batch.custom ? CopyFolder(job) : CopyFromSource(*source, job, game, share * static_cast<long>(i), share);

		if (!copied)
		{
			StageLibrary::DeleteFiles(job.number);
			continue;
		}

		if (batch.replace)
			Replace(job);
		else
			Register(job, from);

		++done;

		LOG("StageImport: %s took %lu ms", job.stage.c_str(), static_cast<unsigned long>(GetTickCount() - began));
	}

	return done;
}

DWORD WINAPI Worker(void* parameter)
{
	const std::unique_ptr<Batch> batch(static_cast<Batch*>(parameter));
	const int total = static_cast<int>(batch->jobs.size());
	const int done = RunBatch(*batch);

	if (done == 1 && total == 1 && batch->replace)
		SetStatus("Stage %d is now %s. Restart the game to see it.", batch->jobs.front().number,
			batch->jobs.front().name.c_str());

	if (done == 1 && total == 1 && !batch->replace)
		SetStatus("%s is installed. Restart the game to see it.", batch->jobs.front().name.c_str());

	if (done != 0 && total > 1)
		SetStatus("%d of %d stage(s) installed. Restart the game to see them.", done, total);

	InterlockedExchange(&g_progress, kWhole);
	InterlockedExchange(&g_finished, 1);
	InterlockedExchange(&g_busy, 0);
	return 0;
}

bool Start(std::unique_ptr<Batch> batch)
{
	if (batch->jobs.empty() || InterlockedCompareExchange(&g_busy, 1, 0) != 0)
		return false;

	InterlockedExchange(&g_progress, 0);

	const HANDLE thread = CreateThread(nullptr, 0, &Worker, batch.get(), 0, nullptr);

	if (thread == nullptr)
	{
		InterlockedExchange(&g_busy, 0);
		SetStatus("Could not start the import.");
		return false;
	}

	batch.release();
	CloseHandle(thread);
	return true;
}

bool Unlisted(int number)
{
	std::vector<GameStages::Own> hidden;
	GameStages::HiddenSnapshot(hidden);

	return std::any_of(hidden.begin(), hidden.end(), [number](const GameStages::Own& own) { return own.number == number && !own.listed; });
}

bool HoldsModel(const std::string& path)
{
	if (GetFileAttributesA((path + "\\" + kModel).c_str()) != INVALID_FILE_ATTRIBUTES)
		return true;

	SetStatus("%s has no %s, so it is not a stage folder.", path.c_str(), kModel);
	return false;
}

bool PickerFull()
{
	if (StagePicker::Room() > 0)
		return false;

	SetStatus("The stage picker is full (%d slots). Take a stage out of it first.", StagePicker::Capacity());
	return true;
}

}

bool StageImport::Scan(const char* folder)
{
	if (IsBusy())
		return false;

	g_offers.clear();
	g_scanFolder.clear();
	g_scanGame.clear();

	const FbGameFolder::Game game = FbGameFolder::Detect(folder);

	if (game == FbGameFolder::Game_None)
	{
		SetStatus("No supported game found in that folder.");
		return false;
	}

	const std::unique_ptr<StageArchive::Source> source = StageArchive::Open(folder);

	if (source == nullptr)
	{
		SetStatus("Could not open the stage files of %s.", FbGameFolder::Name(game));
		return false;
	}

	std::vector<StageArchive::Stage> stages;
	source->Stages(stages);

	for (const StageArchive::Stage& stage : stages)
	{
		Offer offer;
		offer.folder = stage.folder;
		offer.bytes = stage.bytes;
		offer.name = StageNames::English(game, stage.folder);

		if (offer.name.empty())
			TextEncoding::ShiftJisToUtf8(stage.name.c_str(), stage.name.size(), offer.name);

		offer.name = CleanName((offer.name.empty() ? stage.folder : offer.name).c_str()) + FbGameFolder::Tag(game);
		g_offers.push_back(offer);
	}

	if (g_offers.empty())
	{
		SetStatus("No stages found in %s.", FbGameFolder::Name(game));
		return false;
	}

	g_scanFolder = folder;
	g_scanGame = FbGameFolder::Name(game);

	SetStatus("%d stage(s) found in %s.", OfferCount(), g_scanGame.c_str());
	return true;
}

const char* StageImport::ScannedGame()
{
	return g_scanGame.c_str();
}

int StageImport::OfferCount()
{
	return static_cast<int>(g_offers.size());
}

const StageImport::Offer* StageImport::OfferAt(int index)
{
	if (index < 0 || index >= OfferCount())
		return nullptr;

	return &g_offers[index];
}

bool StageImport::InstallMany(const int* indices, const char* const* names, int count)
{
	if (indices == nullptr || names == nullptr || count <= 0 || g_scanFolder.empty() || IsBusy())
		return false;

	std::unique_ptr<Batch> batch(new Batch());
	batch->folder = g_scanFolder;
	batch->custom = false;

	std::vector<int> taken;

	for (int i = 0; i < count; ++i)
	{
		const Offer* const offer = OfferAt(indices[i]);
		const std::string name = CleanName(names[i]);

		if (offer == nullptr || name.empty())
			continue;

		const int number = StageLibrary::FreeNumber(taken);

		if (number < 0)
		{
			SetStatus("%s", kNoNumber);
			break;
		}

		taken.push_back(number);
		batch->jobs.push_back({ offer->folder, name, std::string(), std::string(), number });
	}

	if (batch->jobs.empty())
		return false;

	if (batch->jobs.size() == 1)
		SetStatus("Installing %s...", batch->jobs.front().name.c_str());
	else
		SetStatus("Installing %d stage(s)...", static_cast<int>(batch->jobs.size()));

	return Start(std::move(batch));
}

bool StageImport::InstallFolder(const char* folder, const char* name)
{
	if (folder == nullptr || folder[0] == '\0' || IsBusy() || !HoldsModel(folder))
		return false;

	const std::string path = folder;
	const int number = StageLibrary::FreeNumber(std::vector<int>());

	if (number < 0)
	{
		SetStatus("%s", kNoNumber);
		return false;
	}

	std::unique_ptr<Batch> batch(new Batch());
	batch->folder = path;
	batch->custom = true;
	batch->jobs.push_back({ Leaf(path), CleanName(name), std::string(), path, number });

	SetStatus("Installing %s...", Leaf(path).c_str());
	return Start(std::move(batch));
}

bool StageImport::ReplaceFolder(const char* folder, int number)
{
	if (folder == nullptr || folder[0] == '\0' || IsBusy() || !HoldsModel(folder))
		return false;

	if (number == StageLibrary::kRandomStage || !GameStages::Owns(number))
	{
		SetStatus("Stage %d is not one of the game's stages.", number);
		return false;
	}

	const std::string path = folder;

	std::unique_ptr<Batch> batch(new Batch());
	batch->folder = path;
	batch->custom = true;
	batch->replace = true;
	batch->jobs.push_back({ Leaf(path), std::string(), std::string(), path, number });

	SetStatus("Replacing stage %d with %s...", number, Leaf(path).c_str());
	return Start(std::move(batch));
}

bool StageImport::Restore(int number)
{
	if (IsBusy() || number == StageLibrary::kRandomStage || !GameStages::Owns(number))
		return false;

	StageLibrary::DeleteFiles(number);
	ModFiles::Rescan();
	StageRevision::Bump();

	SetStatus("Stage %d is restored. Restart the game to see it.", number);
	LOG("StageImport: stage %d is restored", number);
	return true;
}

bool StageImport::Remove(int number)
{
	Entry entry = {};

	if (IsBusy() || !StageLibrary::Of(number, entry))
		return false;

	StageLibrary::DeleteFiles(number);
	StageLibrary::Erase(number);
	ModFiles::Rescan();

	SetStatus("%s is removed and its folder deleted. The game drops it after a restart.", entry.name.c_str());
	LOG("StageImport: stage %d '%s' is removed and its files are deleted", number, entry.name.c_str());
	return true;
}

bool StageImport::SetInGame(int number, bool inGame)
{
	Entry entry = {};

	if (IsBusy() || !StageLibrary::Of(number, entry) || entry.shown == inGame)
		return false;

	if (inGame && PickerFull())
		return false;

	StageLibrary::Show(number, inGame);
	SetStatus("%s %s the stage picker. Restart the game to see it.", entry.name.c_str(), inGame ? "added to" :
		"removed from");
	return true;
}

bool StageImport::Unlock(int number, bool unlocked)
{
	if (unlocked && Unlisted(number) && PickerFull())
		return false;

	HiddenStages::SetUnlocked(number, unlocked);
	SetStatus("Stage %d is %s. Restart the game to see it.", number, unlocked ? "unlocked" : "hidden again");
	return true;
}

void StageImport::Update()
{
	if (InterlockedCompareExchange(&g_finished, 0, 1) != 1)
		return;

	ModFiles::Rescan();
	StageRevision::Bump();
}

bool StageImport::IsBusy()
{
	return InterlockedCompareExchange(&g_busy, 0, 0) != 0;
}

int StageImport::Progress()
{
	return static_cast<int>(InterlockedCompareExchange(&g_progress, 0, 0));
}

bool StageImport::NeedsRestart()
{
	return StageRevision::Changed();
}

const char* StageImport::StatusText()
{
	AcquireSRWLockShared(&g_statusLock);
	memcpy(g_shown, g_status, sizeof(g_shown));
	ReleaseSRWLockShared(&g_statusLock);

	return g_shown;
}
