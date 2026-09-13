#pragma once

#include "Core/AsyncFileDialog.h"
#include "Overlay/Window/IWindow.h"
#include "Palette/LivePalette.h"
#include "Palette/PaletteFile.h"

#include <cstdint>
#include <memory>
#include <string>

class PaletteWindow : public IWindow
{
public:
	PaletteWindow(const std::string& title, bool closable, ImGuiWindowFlags windowFlags = 0);

protected:
	void BeforeDraw() override;
	void Draw() override;

private:
	static constexpr int kPlayers = 2;
	static constexpr int kSeats = 4;
	static constexpr int kSubs = PaletteFile::kSubPalettes;
	static constexpr int kUndoDepth = 32;
	static constexpr int kMaxFiles = 64;

	struct Snapshot
	{
		int sub;
		LivePalette::Colours colours;
	};

	struct Side
	{
		LivePalette::Colours colours[kSubs];
		uint8_t composed[kSubs][LivePalette::kBytes];
		bool pulled[kSubs];

		Snapshot history[kUndoDepth];
		int historyCount;

		int chara = -1;
		unsigned generation;
		bool applied;
		int sub;
		int selected = 1;
		int effectEntry = -1;

		char name[48];
		char creator[PaletteFile::kCreatorLength];
		char description[PaletteFile::kDescriptionLength];
		std::string files[kMaxFiles];
		int fileCount;
		int chosen = -1;
		char status[128];

		AsyncFileDialog importDialog;
		AsyncFileDialog exportDialog;
	};

	void DrawSide(int side);
	void DrawPlayer(int player);
	void DrawRemote(int player);
	void DrawSubPalettes(int player);
	void DrawParts(int player);
	void DrawPartStock(int player, int part);
	void DrawPartPick(int player, int part);
	void DrawSwatches(int player);
	void DrawGroupedSwatches(int player);
	void DrawFlatSwatches(int player);
	void DrawGrid(int player, const unsigned char* entries, int count);
	void DrawPicker(int player);
	void DrawPickerButtons(int player);
	void DrawEffects(int player);
	void DrawEffectGrid(int player, const unsigned char* entries, int count);
	void DrawEffectPicker(int player);
	void DrawFiles(int player);
	void DrawFileChooser(int player);
	void DrawPngButtons(int player);

	bool IsJunk(int player, int entry) const;

	void PullBaseline(int player, bool force);
	void Record(int player);
	void Undo(int player);
	void Apply(int player);
	void Refresh(int player);

	void ApplyImportedColours(int player, const uint8_t* colours);
	bool Save(int player);
	bool Load(int player, const char* name);
	void RefreshFiles(int player);
	void SelectFile(int player, const char* file);
	void Bare(int player);
	void Remove(int player);

	void PollPngDialogs(int player);
	void CompleteImportPng(int player, const std::string& path);
	void CompleteExportPng(int player, const std::string& path);

	void Adopt(int player, int chara);
	void LoadCreator();

	void StartFlash(int player);
	void RunFlash();

	Side& SideOf(int player) { return *m_sides[player]; }
	const Side& SideOf(int player) const { return *m_sides[player]; }

	std::unique_ptr<Side> m_sides[kSeats];
	bool m_creatorLoaded = false;
	int m_flashFrames = 0;
	int m_flashPlayer = -1;
};
