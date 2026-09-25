#pragma once

#include <cstddef>
#include <cstdint>

namespace GameOffsets
{
	namespace Files
	{
		inline constexpr const char* kReaderOpenAnchor = "./grpdat/Title/title00.pat";
		inline constexpr uint16_t kReaderOpenStackBytes = 0x10;

		inline constexpr size_t kMaxReaderFromMemoryLength = 0x200;
		inline constexpr uint8_t kReaderTypeOffset = 0x1C;
		inline constexpr uint32_t kReaderTypeMemory = 2;
		inline constexpr uint16_t kReaderFromMemoryStackBytes = 0x08;

		inline constexpr uint8_t kStoreDword = 0xC7;
		inline constexpr uint8_t kCall = 0xE8;
		inline constexpr size_t kCallLength = 5;
		inline constexpr size_t kLoadCallWindow = 0x30;
		inline constexpr int kLeastLoadCallers = 50;

		inline constexpr uintptr_t kReaderData = 0x30;
		inline constexpr uintptr_t kReaderSize = 0x28;
		inline constexpr size_t kReaderBytes = 0x70;
		inline constexpr uint32_t kReaderTypeClosed = 5;
		inline constexpr int kOpenDecrypt = 1;
		inline constexpr int kOpenShare = 1;
		inline constexpr int kOpenFlags = 0;

		inline constexpr int kLeastCtorCallers = 100;

		inline constexpr const char* kTextureExtensionAnchor = ".png";
		inline constexpr const char* kTextureExtensionCheck = ".tga";
		inline constexpr const char* kKernelLibrary = "KERNEL32.dll";
		inline constexpr const char* kFileAttributesImport = "GetFileAttributesA";
	}

	namespace Battle
	{
		inline constexpr const char* kStepAnchor = "BattleProc Create";
		inline constexpr size_t kCallLength = 5;
		inline constexpr size_t kUpdateWindow = 0x30;
		inline constexpr int kLeastUpdateVotes = 3;
		inline constexpr int kSimFlag = 0;
		inline constexpr int kFirstFlag = 1;

		inline constexpr const char* kTrainingNative = "IsTrainingBattle";
		inline constexpr uintptr_t kFrameCounter = 0x34;
		inline constexpr uintptr_t kMode = 0xC8;
		inline constexpr uintptr_t kSubMode = 0xCC;
		inline constexpr int kModeSingle = 0;
		inline constexpr int kModeTraining = 3;
		inline constexpr int kSubModeTraining = 1;

		inline constexpr uint8_t kLoadEcxGlobal[] = { 0x8B, 0x0D };
		inline constexpr uint8_t kTestEcx[] = { 0x85, 0xC9 };
		inline constexpr size_t kNullTestAt = 6;
		inline constexpr size_t kNullTestLength = 8;
		inline constexpr int kLeastSessionTicks = 2;

		inline constexpr const char* kPauseAnchor = "PLAYER %d PAUSE";
		inline constexpr uint8_t kLoadEcxImmediate = 0xB9;
		inline constexpr int kPauseLeastLoads = 10;
		inline constexpr int kPauseMajority = 2;
		inline constexpr uintptr_t kPauseState = 0x00;
	}

	namespace Objects
	{
		inline constexpr const char* kActivePlayerNative = "IsActivePlayer";
		inline constexpr size_t kStrideSearchWindow = 16;
		inline constexpr size_t kIndexerLength = 33;
		inline constexpr size_t kIndexedArrays = 2;
		inline constexpr uint8_t kImul = 0x69;
		inline constexpr uint8_t kAddEax = 0x05;
		inline constexpr uint8_t kAddGroup = 0x81;
		inline constexpr uint8_t kStoreByte = 0xC6;
		inline constexpr uint8_t kLoadEcxGlobal[] = { 0x8B, 0x0D };
		inline constexpr size_t kLoadEcxGlobalLength = 6;
		inline constexpr uint8_t kCountUp[] = { 0xFF, 0x05 };
		inline constexpr int kCharaSlots = 14;

		inline constexpr uintptr_t kActive = 0x864;
		inline constexpr uint8_t kEffectSpawnStore[] = { 0x64, 0x08, 0x00, 0x00, 0x01 };
		inline constexpr size_t kEffectListWindow = 0x20;
		inline constexpr int kMostEffects = 2000;
		inline constexpr uintptr_t kEffectPointers = 0x04;

		inline constexpr uintptr_t kPositionX = 0x64;
		inline constexpr uintptr_t kPositionY = 0x68;
		inline constexpr uintptr_t kOffsetX = 0x70;
		inline constexpr uintptr_t kOffsetY = 0x74;
		inline constexpr uintptr_t kExistFlags = 0x84;
		inline constexpr uintptr_t kFacing = 0x6EC;
		inline constexpr uintptr_t kFrame = 0x6FC;

		inline constexpr uint32_t kNoPush = 0x100;
		inline constexpr uint32_t kNoHurt = 0x200;
		inline constexpr uint32_t kNoAttack = 0x400;

		inline constexpr uintptr_t kFullInvulnLevel = 0x2A5;
		inline constexpr uintptr_t kFullInvulnGate = 0x2A7;
		inline constexpr uint8_t kFullInvulnLeast = 3;
		inline constexpr uintptr_t kDeathFlag = 0x2A4;
		inline constexpr uint8_t kDead = 1;
		inline constexpr uintptr_t kDeathRecord = 0x5D4;
		inline constexpr uintptr_t kRecordTime = 0x0C;
		inline constexpr uintptr_t kStrikeTimerHard = 0x2BA;
		inline constexpr uintptr_t kThrowTimerHard = 0x2BB;
		inline constexpr uintptr_t kLinkedObject = 0x4A4;
		inline constexpr uintptr_t kFrameRecord = 0xAC;
		inline constexpr uintptr_t kFrameInvuln = 0x0D;
		inline constexpr uint8_t kInvulnStrike = 3;
		inline constexpr uint8_t kInvulnThrow = 4;
		inline constexpr uint8_t kInvulnBoth = 5;

		inline constexpr uintptr_t kFrameAttackData = 0xB0;
		inline constexpr uintptr_t kFrameNormalCount = 0xB6;
		inline constexpr uintptr_t kFrameAttackCount = 0xB7;
		inline constexpr uintptr_t kFrameNormalBoxes = 0xC0;
		inline constexpr uintptr_t kFrameAttackBoxes = 0xC4;

		inline constexpr int kPushBox = 0;
		inline constexpr int kLastHurtBox = 8;
		inline constexpr int kClashBox = 11;
	}

	namespace Palette
	{
		inline constexpr uintptr_t kCharaOwner = 0x708;
		inline constexpr uintptr_t kOwnerFromLink = 0x518;
		inline constexpr uintptr_t kOwnerColour = 0x08;
		inline constexpr uintptr_t kOwnerEffectColours = 0x118;
		inline constexpr uintptr_t kOwnerShared = 0x51C;
		inline constexpr uintptr_t kOwnerChara = 0x548;
		inline constexpr uintptr_t kOwnerPaletteChara = 0x54C;
		inline constexpr uintptr_t kOwnerTexture = 0x558;
		inline constexpr uintptr_t kSharedTexture = 0x15050;
		inline constexpr uintptr_t kTextureInterface = 0x0C;
		inline constexpr int kMostCharas = 64;

		inline constexpr int kSubPalettes = 4;
		inline constexpr int kColours = 256;
		inline constexpr size_t kPageBytes = 1024;
		inline constexpr unsigned kTextureWidth = 256;
		inline constexpr unsigned kTextureRows = 8;

		inline constexpr size_t kFileHeader = 16;
		inline constexpr int kStockLimit = 42;
	}

	namespace SaveData
	{
		inline constexpr const wchar_t* kIncrementAnchor = L"!\"AchievementCountIncrement_Base type OutOfRange.\"";
		inline constexpr const wchar_t* kSetAnchor = L"!\"SetAchievementCount_Base type OutOfRange.\"";
		inline constexpr uint16_t kIncrementStackBytes = 0x08;
		inline constexpr uint16_t kSetStackBytes = 0x0C;

		inline constexpr uint8_t kTypeLowerCheck[] = { 0x83, 0x7D, 0x08, 0x00, 0x7C };
		inline constexpr size_t kTypeUpperCheckAt = 6;
		inline constexpr uint8_t kTypeUpperCheck[] = { 0x83, 0x7D, 0x08 };
		inline constexpr uint8_t kJumpLess = 0x7C;
		inline constexpr int kSharedType = -1;
	}

	namespace Asserts
	{
		inline constexpr const char* kRuntimeLibrary = "api-ms-win-crt-runtime-l1-1-0.dll";
		inline constexpr const char* kAssertImport = "_wassert";

		inline constexpr uint8_t kCallSlot[] = { 0xFF, 0x15 };
		inline constexpr size_t kCallLength = 6;
		inline constexpr uint8_t kCleanup[] = { 0x83, 0xC4, 0x0C };
		inline constexpr uint8_t kPushImm = 0x68;
		inline constexpr size_t kPushLength = 5;
		inline constexpr uint8_t kPushByte = 0x6A;
		inline constexpr size_t kPushByteLength = 2;
		inline constexpr uint32_t kLongestLine = 0xFFFF;
		inline constexpr size_t kLongestBlock = kPushLength * 3 + kCallLength + sizeof(kCleanup);

		inline constexpr uint8_t kCompareRegister = 0x83;
		inline constexpr uint8_t kCompareModRmMask = 0xF8;
		inline constexpr uint8_t kCompareModRm = 0xF8;
		inline constexpr size_t kCheckLength = 5;
		inline constexpr size_t kBoundAt = 2;
		inline constexpr size_t kJumpAt = 3;
		inline constexpr size_t kJumpDistanceAt = 4;
		inline constexpr uint8_t kJumpBelow = 0x72;
		inline constexpr uint8_t kJumpLess = 0x7C;

		inline constexpr uint8_t kMoveEax = 0xB8;
		inline constexpr size_t kJumpOpcodeAt = 5;
		inline constexpr uint8_t kJump = 0xE9;
		inline constexpr size_t kPatchLength = 10;
		inline constexpr uint8_t kFill = 0xCC;
		inline constexpr size_t kEntryWindow = 0x60;

		struct CharacterCheck
		{
			const wchar_t* file;
			const wchar_t* expression;
		};

		inline constexpr const wchar_t* kSaveDataFile = L"\\savedatacs.cpp";
		inline constexpr const wchar_t* kBattleStatusFile = L"\\networkbattleplayerstatusdata.cpp";

		inline constexpr CharacterCheck kCharacterChecks[] = {
			{ kSaveDataFile, L"CharID < SAVEDATACS_CHARACTER_NUM" },
			{ kSaveDataFile, L"CharID < SAVEDATACS_CHARACTER_NUM+1" },
			{ kBattleStatusFile, L"chara_no < MAX_NETWORK_BATTLE_CHARA_NUM" },
			{ kBattleStatusFile, L"src_chara_no < MAX_NETWORK_BATTLE_CHARA_NUM" },
			{ kBattleStatusFile, L"dst_chara_no < MAX_NETWORK_BATTLE_CHARA_NUM" },
		};

		inline constexpr const wchar_t* kUnlockedColourCheck = L"ColorID < SAVEDATACS_COLOR_NUM";
		inline constexpr uint16_t kColourGetterStackBytes = 0x08;
	}

	namespace Scenes
	{
		inline constexpr const char* kReturnTitleAnchor = "ReturnTitle";

		inline constexpr uint8_t kStoreGlobal[] = { 0xC7, 0x05 };
		inline constexpr size_t kStoreLength = 10;
		inline constexpr size_t kStoreAddressAt = 2;
		inline constexpr size_t kStoreValueAt = 6;
		inline constexpr size_t kTitleStores = 4;
		inline constexpr uint8_t kJump = 0xE9;
		inline constexpr size_t kJumpLength = 5;

		inline constexpr size_t kEnteringWindow = 0x40;
		inline constexpr int kLeastEnteringVotes = 32;
		inline constexpr int kEnteringMajority = 4;

		inline constexpr uint8_t kJumpTable[] = { 0xFF, 0x24 };
		inline constexpr size_t kJumpTableAddressAt = 3;
		inline constexpr size_t kJumpTableLength = 7;
		inline constexpr size_t kLeastSceneCases = 40;
		inline constexpr size_t kMostSceneCases = 300;

		inline constexpr uintptr_t kSceneId = 0x08;

		inline constexpr uint8_t kCountdownCompare[] = { 0x83, 0xF8, 0x1E };
		inline constexpr uint8_t kLoadEax = 0xA1;
		inline constexpr uint8_t kIncrementEax = 0x40;
		inline constexpr uint8_t kStoreEax = 0xA3;
		inline constexpr size_t kCountdownLength = 11;
		inline constexpr size_t kCountdownIncrementAt = 5;
		inline constexpr size_t kCountdownStoreAt = 6;
		inline constexpr int kLeastCountdownVotes = 3;
		inline constexpr int kCountdownMajority = 2;
	}

	namespace Draw
	{
		inline constexpr uint8_t kCallSlot = 0xFF;
		inline constexpr uint8_t kCallSlotDigit = 2;
		inline constexpr size_t kTypeCallLength = 3;
		inline constexpr size_t kTypeCompareWindow = 12;
		inline constexpr uint8_t kCompareEax[] = { 0x83, 0xF8 };
		inline constexpr uint8_t kCompareByte = 0x80;
		inline constexpr uint8_t kCompareDigit = 7;
		inline constexpr size_t kActiveTestLength = 7;
		inline constexpr uint8_t kActiveDisplacement[] = { 0x64, 0x08, 0x00, 0x00 };
		inline constexpr size_t kTypeSlot = 2;
		inline constexpr int kCharacterType = 1;
		inline constexpr int kEffectType = 2;
	}

	namespace Bloom
	{
		inline constexpr const char* kBrightnessName = "g_Blightness";
		inline constexpr size_t kCallLength = 5;
		inline constexpr uint16_t kPassStackBytes = 0;
		inline constexpr uint8_t kPushImmediate = 0x68;
		inline constexpr size_t kPushLength = 5;
		inline constexpr uint8_t kLoadScalar[] = { 0xF3, 0x0F, 0x10, 0x05 };
		inline constexpr size_t kMultiplyScalarAt = 8;
		inline constexpr uint8_t kMultiplyScalar[] = { 0xF3, 0x0F, 0x59, 0x05 };
		inline constexpr size_t kScalarReadLength = 12;
		inline constexpr size_t kAlphaWindow = 0x320;
		inline constexpr int kFullPercent = 100;
		inline constexpr int kMostPercent = 200;
		inline constexpr float kMostBrightness = 2.0f;
		inline constexpr float kMostAlpha = 1.0f;
	}

	namespace Light
	{
		inline constexpr const char* kColourImage = "stage_color.img";
		inline constexpr const char* kSpecularImage = "stage_specular.img";
		inline constexpr const char* kBokashiImage = "stage_bokashi_alpha.img";

		inline constexpr uint8_t kPushImmediate = 0x68;
		inline constexpr size_t kPushLength = 5;
		inline constexpr uint8_t kCall = 0xE8;
		inline constexpr uint8_t kLea = 0x8D;
		inline constexpr uint8_t kLeaEaxDisp32Mask = 0xC7;
		inline constexpr uint8_t kLeaEaxDisp32 = 0x80;
		inline constexpr uint8_t kLeaEcx = 0x88;
		inline constexpr size_t kLeaLength = 6;
		inline constexpr int kRegisterShift = 3;
		inline constexpr uint8_t kRegisterMask = 7;
		inline constexpr int kRegisters = 8;
		inline constexpr uint8_t kMov = 0x8B;
		inline constexpr uint8_t kMovEcxFirst = 0xC8;
		inline constexpr uint8_t kMovEcxLast = 0xCF;

		inline constexpr uint8_t kPushByte = 0x6A;
		inline constexpr uint8_t kBankStride = 0x20;
		inline constexpr size_t kVectorPushes = 12;
		inline constexpr size_t kCtorPushAt = 5;
		inline constexpr size_t kCountPushAt = 10;
		inline constexpr uint8_t kSetVtable[] = { 0xC7, 0x41, 0x04 };
		inline constexpr uintptr_t kImageOffset = 4;
		inline constexpr size_t kReaderSlot = 1;

		inline constexpr int kColourChannels = 3;
		inline constexpr int kFullPercent = 100;
		inline constexpr int kWhite = 255;
		inline constexpr int kNone = 0;
	}

	namespace Cockpit
	{
		inline constexpr const char* kViewNative = "Cockpit_SetView";
		inline constexpr uint8_t kLoadEcxGlobal[] = { 0x8B, 0x0D };
		inline constexpr uint8_t kHideStore[] = { 0xC7, 0x41 };
		inline constexpr size_t kHideOffsetAt = 2;
		inline constexpr size_t kHideValueAt = 3;
		inline constexpr size_t kHideStoreLength = 7;
		inline constexpr uint32_t kViewShown = 0;
		inline constexpr uint32_t kViewHidden = 1;
	}

	namespace Roster
	{
		inline constexpr const char* kTableAnchor = "./System/BtlCharaTbl.txt";
		inline constexpr const char* kNetworkNative = "GetMvNetworkInfo";
		inline constexpr uint8_t kLoadEcx = 0xB9;
		inline constexpr size_t kThisWindow = 0x20;
		inline constexpr uint8_t kCompareByteGlobal[] = { 0x80, 0x3D };
		inline constexpr size_t kCompareLength = 7;

		inline constexpr uintptr_t kRecordsBegin = 0x04;
		inline constexpr uintptr_t kRecordsEnd = 0x08;
		inline constexpr uintptr_t kRecordBytes = 0xC0;
		inline constexpr uintptr_t kAlive = 0x68;
		inline constexpr uintptr_t kUnselect = 0x6C;
		inline constexpr int kMostCharas = 64;

		inline constexpr uintptr_t kGridColumns = 0x338;
		inline constexpr uintptr_t kGridRows = 0x33C;
		inline constexpr uintptr_t kGridBegin = 0x340;
		inline constexpr uintptr_t kGridRowBytes = 0x0C;
		inline constexpr int kMostGridSide = 16;
		inline constexpr int32_t kEmptyCell = -1;
	}

	namespace Colours
	{
		inline constexpr int kCharas = 28;
		inline constexpr int kMostCharas = 64;
		inline constexpr int kSlots = 0x30;
		inline constexpr int kPickerSlots = 42;
		inline constexpr int kStockSlots = 25;
		inline constexpr int kTableFirst = 0x0A;
		inline constexpr uint8_t kNarrowRange = static_cast<uint8_t>(kStockSlots - 1 - kTableFirst);
		inline constexpr uint8_t kWideRange = static_cast<uint8_t>(kPickerSlots - 1 - kTableFirst);
		inline constexpr uint8_t kGranted = 1;

		inline constexpr uint8_t kCompareEax[] = { 0x83, 0xF8 };
		inline constexpr size_t kRangeAt = 2;
		inline constexpr size_t kJumpAt = 3;
		inline constexpr size_t kCompareLength = 4;
		inline constexpr uint8_t kJumpAbove = 0x77;
		inline constexpr uint8_t kLoadAddress = 0x8D;
		inline constexpr size_t kLoadAddressLength = 3;

		inline constexpr uint8_t kLoadByte = 0x8A;
		inline constexpr uint8_t kModRmMask = 0xC0;
		inline constexpr uint8_t kDisplaced = 0x80;
		inline constexpr uint8_t kRegisterMask = 0x07;
		inline constexpr uint8_t kScaledIndex = 0x04;
		inline constexpr size_t kLoadByteLength = 3;
		inline constexpr uint8_t kCompareEaxImmediate = 0x3D;
		inline constexpr size_t kCompareImmediateLength = 5;
	}

	namespace Netplay
	{
		inline constexpr uint8_t kPushImmediate = 0x68;
		inline constexpr uint8_t kImulFrame = 0x6B;
		inline constexpr uint8_t kImulFrameModRm = 0x85;
		inline constexpr uint8_t kImulFrameModRmMask = 0xC7;
		inline constexpr size_t kImulFrameLength = 7;

		inline constexpr uint8_t kPlayerRecordBytes = 0x30;
		inline constexpr uintptr_t kPlayerType = 0x04;
		inline constexpr uintptr_t kPlayerNumber = 0x08;
		inline constexpr int32_t kPlayerLocal = 0;
		inline constexpr int kArgPlayers = 1;
		inline constexpr int kArgRecords = 2;
		inline constexpr int kArgSteamIds = 6;
		inline constexpr int kMostPlayers = 4;

		inline constexpr const char* kSteamLibrary = "steam_api.dll";
		inline constexpr int kMostAttempts = 20;
		inline constexpr int kSendReliable = 8;
		inline constexpr int kResultOk = 1;
		inline constexpr size_t kIdentityBytes = 136;
		inline constexpr int32_t kIdentitySteamId = 16;
		inline constexpr int32_t kIdentitySteamIdBytes = 8;
		inline constexpr uintptr_t kMessageData = 0x00;
		inline constexpr uintptr_t kMessageSize = 0x04;
		inline constexpr uintptr_t kMessageIdentityOffsets[] = { 0x10, 0x0C };
	}

	namespace Ggpo
	{
		inline constexpr const char* kBackendTypeName = ".?AVPeer2PeerBackend@@";
		inline constexpr uintptr_t kTypeDescriptorFromName = 8;
		inline constexpr uintptr_t kLocatorTypeDescriptor = 12;
		inline constexpr uintptr_t kLocatorSignature = 0;
		inline constexpr uintptr_t kLocatorOffset = 4;
		inline constexpr uintptr_t kVTableFromLocator = 4;

		inline constexpr uintptr_t kPlayerEndpoints = 0xBD8;
		inline constexpr uintptr_t kSpectatorEndpoints = 0xBDC;
		inline constexpr uintptr_t kSpectatorCount = 0x4BA5C;
		inline constexpr uintptr_t kSynchronizing = 0x4BA64;
		inline constexpr uintptr_t kPlayerCount = 0x4BA68;

		inline constexpr size_t kEndpointBytes = 0x2574;
		inline constexpr uintptr_t kEndpointUdp = 0x004;
		inline constexpr uintptr_t kEndpointSteamLow = 0x018;
		inline constexpr uintptr_t kEndpointSteamHigh = 0x01C;
		inline constexpr uintptr_t kEndpointQueue = 0x02C;
		inline constexpr uintptr_t kEndpointRoundTrip = 0xA70;
		inline constexpr uintptr_t kEndpointKbpsSent = 0xA7C;
		inline constexpr uintptr_t kEndpointState = 0xA98;
		inline constexpr uintptr_t kEndpointLocalBehind = 0xAA8;
		inline constexpr uintptr_t kEndpointRemoteBehind = 0xAAC;
		inline constexpr uintptr_t kEndpointPendingOutput = 0x15B8;

		inline constexpr uint32_t kSteamIdIndividualHigh = 0x01100001;
		inline constexpr int kMostPlayers = 4;
		inline constexpr int kMostSpectators = 32;

		inline constexpr uint32_t kStateSyncing = 0;
		inline constexpr uint32_t kStateSynchronized = 1;
		inline constexpr uint32_t kStateRunning = 2;
		inline constexpr uint32_t kStateDisconnected = 3;
	}

	namespace Render
	{
		inline constexpr int kSceneWidth = 1280;
		inline constexpr int kSceneHeight = 720;

		inline constexpr const char* kStageDrawAnchor = "TinyFXAA";
		inline constexpr const char* kStageSpecularAnchor = "TecBgSpeculer";
		inline constexpr uint8_t kGlobalCompare[] = { 0x83, 0x3D };
		inline constexpr size_t kGlobalCompareLength = 7;
		inline constexpr int kGateCompares = 3;
		inline constexpr size_t kGateWindow = 0x80;
		inline constexpr uint8_t kSkipBranch[] = { 0x84, 0xC0, 0x0F, 0x84 };
		inline constexpr size_t kSkipTestLength = 2;
		inline constexpr size_t kSkipBranchLength = 6;
		inline constexpr size_t kSkipOpcodeLength = 2;
		inline constexpr uint8_t kSkipAlways[] = { 0x90, 0xE9 };

		inline constexpr uint8_t kCompareByteGlobal[] = { 0x80, 0x3D };
		inline constexpr size_t kCompareByteLength = 7;

		inline constexpr uint8_t kCallMemory = 0xFF;
		inline constexpr uint8_t kCallDisp32First = 0x90;
		inline constexpr uint8_t kCallDisp32Last = 0x97;
		inline constexpr uint8_t kStretchRectSlot[] = { 0x88, 0x00, 0x00, 0x00 };
		inline constexpr size_t kStretchRectLength = 6;
		inline constexpr uint8_t kPushGlobal[] = { 0xFF, 0x35 };
		inline constexpr size_t kPushWindow = 0x20;

		inline constexpr uint8_t kLoadSamples[] = { 0x8B, 0x9E };
		inline constexpr size_t kLoadSamplesLength = 6;
		inline constexpr uint8_t kTestSamples[] = { 0x85, 0xDB, 0x74 };
		inline constexpr size_t kTestSamplesWindow = 8;
		inline constexpr uint8_t kStoreSamples[] = { 0x89, 0x1D };
		inline constexpr size_t kStoreSamplesLength = 6;
		inline constexpr uintptr_t kSurfaceFields = 0x20;
		inline constexpr uint8_t kNoSamples[] = { 0x33, 0xDB, 0x90, 0x90, 0x90, 0x90 };
	}

	namespace Input
	{
		inline constexpr const char* kStickNative = "GetStickHold";
		inline constexpr uint8_t kLoadLeverByte[] = { 0x0F, 0xB6 };
		inline constexpr uint8_t kLoadLeverModRmMask = 0xC7;
		inline constexpr uint8_t kLoadLeverModRm = 0x86;
		inline constexpr size_t kLoadLeverOffsetAt = 3;
		inline constexpr size_t kLoadLeverLength = 7;
		inline constexpr int kLeverShift = 24;
		inline constexpr uint32_t kButtonMask = 0x00FFFFFF;
		inline constexpr const char* kXInputLibrary = "xinput1_3.dll";
		inline constexpr int kXInputGetStateOrdinal = 2;
	}

	namespace Meter
	{
		inline constexpr uintptr_t kTeamSlot = 0x00;
		inline constexpr uintptr_t kTeamPoint = 0x14;

		inline constexpr const char* kComboNatives[] = { "GetComboInfo", "AddComboCount", "ComboView_Set" };
		inline constexpr uintptr_t kComboRecordSkip = 0x08;
		inline constexpr uintptr_t kComboActive = 0x08;
		inline constexpr uintptr_t kComboHits = 0x20;

		inline constexpr const char* kFontPath = "grpdat/Font/NewLodinB_30.fnt";

		namespace Chara
		{
			inline constexpr uintptr_t kObjectId = 0x08;
			inline constexpr uintptr_t kPattern = 0x1C;
			inline constexpr uintptr_t kHitstop = 0x298;
			inline constexpr uintptr_t kInGuard = 0x29E;
			inline constexpr uintptr_t kShield = 0x2A0;
			inline constexpr uintptr_t kFullInvulnLevel = 0x2A5;
			inline constexpr uintptr_t kFullInvulnGate = 0x2A7;
			inline constexpr uint8_t kFullInvulnLeast = 3;
			inline constexpr uintptr_t kInvulnCountdowns = 0x2B8;
			inline constexpr uintptr_t kReaction = 0x2D8;
			inline constexpr uintptr_t kReactionStun = 0x2DC;
			inline constexpr uintptr_t kOwner = 0x4AC;
			inline constexpr uintptr_t kOwnedObjects = 0x4B4;
			inline constexpr uintptr_t kSide = 0x4EC;
			inline constexpr uintptr_t kMoveAble = 0x4F4;
			inline constexpr int kMoveAbleRecords = 2;
			inline constexpr uintptr_t kRecordStride = 0x10;
			inline constexpr uintptr_t kRecordTime = 0x0C;
			inline constexpr uintptr_t kCancelBoost = 0x514;
			inline constexpr uint8_t kCancelUnset = 0xFF;
			inline constexpr uintptr_t kStanceOverride = 0x524;
			inline constexpr uintptr_t kHitCheck = 0x554;
			inline constexpr uintptr_t kArmor = 0x618;
			inline constexpr uintptr_t kRunning = 0x722;
			inline constexpr uintptr_t kMvCountFrame = 0x72C;
			inline constexpr uintptr_t kCommand = 0x73C;
			inline constexpr uintptr_t kMoveCodes = 0x75C;

			inline constexpr uintptr_t kFrameRecord = 0xAC;
			inline constexpr uintptr_t kFrameStance = 0x0C;
			inline constexpr uintptr_t kFrameInvuln = 0x0D;
			inline constexpr uintptr_t kFrameCancelNormal = 0x0E;
			inline constexpr uintptr_t kFrameCancelSpecial = 0x0F;
			inline constexpr uintptr_t kFrameFree = 0x11;

			inline constexpr uint8_t kStanceAir = 1;
			inline constexpr uint8_t kCancelAlways = 2;

			inline constexpr uint8_t kInvulnHighMid = 1;
			inline constexpr uint8_t kInvulnLowMid = 2;
			inline constexpr uint8_t kInvulnStrike = 3;
			inline constexpr uint8_t kInvulnThrow = 4;
			inline constexpr uint8_t kInvulnBoth = 5;

			inline constexpr uint8_t kHitHead = 0x01;
			inline constexpr uint8_t kHitBody = 0x02;
			inline constexpr uint8_t kHitLegs = 0x04;
			inline constexpr uint8_t kHitFireBall = 0x08;
			inline constexpr uint8_t kHitThrow = 0x10;
			inline constexpr uint8_t kHitAirDive = 0x40;
		}

		namespace Codes
		{
			inline constexpr uint32_t kAttackBits = 0x01 | 0x02 | 0x04 | 0x20 | 0x40 | 0x80;
			inline constexpr uint32_t kRecovery = 0x10;
			inline constexpr uint32_t kJump = 0x400000;
			inline constexpr uint32_t kFromAssault = 0x10;
			inline constexpr uint32_t kEnemyAntenStop = 0x100000;
			inline constexpr uint32_t kAnten = 0x8000;
			inline constexpr uint32_t kSousaiMuteki = 0x1000;
		}
	}

	namespace Camera
	{
		inline constexpr const char* kPositionNative = "GetCameraPosition";
		inline constexpr uint8_t kLoadEaxGlobal = 0xA1;
		inline constexpr uintptr_t kElementBytes = 0x18;
		inline constexpr uintptr_t kElementX = 0x08;
		inline constexpr uintptr_t kElementY = 0x0C;
		inline constexpr uintptr_t kElementZoom = 0x14;
		inline constexpr int kElementCount = 3;

		inline constexpr float kReferenceWidth = 1280.0f;
		inline constexpr float kReferenceHeight = 720.0f;
		inline constexpr float kReferenceOriginX = 640.0f;
		inline constexpr float kReferenceOriginY = 640.0f;
		inline constexpr float kSubpixels = 128.0f;
	}

	namespace Stages
	{
		inline constexpr const char* kListAnchor = "./bg/BgList.txt";
		inline constexpr const char* kRandomFilterAnchor = "RANDOMFILTER_MAX_OVER";
		inline constexpr size_t kRecordBytes = 0x234;
		inline constexpr uintptr_t kRecordSelectDisable = 0x60;
		inline constexpr uintptr_t kRecordRandomDisable = 0x64;
		inline constexpr uintptr_t kRecordVsDisable = 0x68;
		inline constexpr size_t kCountStoreWindow = 0x20;
		inline constexpr size_t kTinyFunction = 0x20;

		inline constexpr int kTableReferences = 10;
		inline constexpr int kListReferences = 4;
		inline constexpr int kBoundSites = 8;
		inline constexpr int kLimitSites = 1;
		inline constexpr int kLeastArrayCells = 2;
		inline constexpr int kMostArrayCells = 12;

		inline constexpr uint8_t kBelowCount = 0x63;
		inline constexpr uint8_t kAtCount = 0x64;

		inline constexpr int32_t kExclusionBase = -0x204;
		inline constexpr int kExclusionCells = 4;
		inline constexpr uint8_t kFilterRead[] = { 0x0F, 0xB6, 0x42, 0x40, 0x85, 0xC0, 0x74 };
		inline constexpr size_t kFilterReadLength = 8;
		inline constexpr uint8_t kFilterIndexCompare[] = { 0x81, 0x7D, 0xFC };
		inline constexpr uint32_t kFilterEntries = 0x80;
	}

	namespace Cards
	{
		inline constexpr const char* kFirstSheetAnchor = "grpdat/CSel/stage_thumb00";
		inline constexpr const char* kSecondSheetAnchor = "grpdat/CSel/stage_thumb01";
		inline constexpr const char* kSecondSheetFile = "grpdat/CSel/stage_thumb01.dds";
		inline constexpr size_t kStoreWindow = 0x20;
		inline constexpr uint8_t kCall = 0xE8;
		inline constexpr size_t kCallLength = 5;
		inline constexpr uint8_t kLoadEcxName = 0xB9;
		inline constexpr uint8_t kStoreSheet[] = { 0x89, 0x86 };
		inline constexpr size_t kStoreSheetDispAt = 2;
		inline constexpr size_t kStoreSheetLength = 6;

		inline constexpr uint8_t kClampAndFetch[] = { 0x33, 0xC0, 0x83, 0xF9, 0x01, 0x0F, 0x4E, 0xC1, 0x8B, 0x84, 0x83 };
		inline constexpr size_t kFetchDispAt = 11;
		inline constexpr size_t kClampLength = 15;
		inline constexpr uint8_t kDivide[] = { 0xB8, 0x31, 0x0C, 0xC3, 0x30, 0xF3, 0x0F, 0x10, 0x4B, 0x4C, 0xF7, 0xEF,
			0x0F, 0x57, 0xC0, 0xC1, 0xFA, 0x02, 0x8B, 0xCA, 0xC1, 0xE9, 0x1F, 0x03, 0xCA, 0x6B, 0xC1, 0xEB, 0x03, 0xF8 };
		inline constexpr uint8_t kNop = 0x90;

		inline constexpr int kPerSheet = 21;
		inline constexpr int kPerRow = 7;
		inline constexpr int kStockRows = 3;
		inline constexpr uint32_t kCellHeight = 336;
		inline constexpr uint32_t kCellWidth = 144;
		inline constexpr int kStockFirstFree = 31;
		inline constexpr int kArtX = 20;
		inline constexpr int kArtY = 3;
		inline constexpr int kArtWidth = 104;
		inline constexpr int kArtHeight = 330;
		inline constexpr size_t kDdsHeightAt = 12;
		inline constexpr size_t kDdsHeaderBytes = 16;
	}

	namespace Tint
	{
		inline constexpr const char* kLoaderAnchor = "%s\\%s\\bg.fbx";
		inline constexpr uint8_t kMoveFromEcx = 0x8B;
		inline constexpr uint8_t kFromEcxMask = 0xC7;
		inline constexpr uint8_t kFromEcxBase = 0xC1;
		inline constexpr uint8_t kStoreGlobal = 0x89;
		inline constexpr uint8_t kStoreGlobalModRm = 0x05;
		inline constexpr size_t kEntryWindow = 0x20;

		inline constexpr uintptr_t kTint = 0x3B4;
		inline constexpr uintptr_t kTimeAt = 0x4;
		inline constexpr uintptr_t kTotalAt = 0x6;
		inline constexpr uintptr_t kInAt = 0x8;
		inline constexpr uintptr_t kTypeAt = 0xA;
		inline constexpr uint8_t kTypeSolid = 3;
		inline constexpr int16_t kHoldFrames = 3;
		inline constexpr int kSeats = 4;
	}

	namespace Pictures
	{
		inline constexpr const char* kVsPrefix = "grpdat\\vsscreen\\vs_bg\\vs_demo_bg";
		inline constexpr const char* kMenuPrefix = "grpdat\\menucommon\\menu_bg";
		inline constexpr const char* kSuffix = ".pat";
		inline constexpr int kDonorNumber = 1;

		inline constexpr uint8_t kTextureTag[] = { 'P', 'G', 'T', '2' };
		inline constexpr uint8_t kDdsMagic[] = { 'D', 'D', 'S', ' ' };
		inline constexpr size_t kTextureBytesAt = 4;
		inline constexpr size_t kTextureWidthAt = 8;
		inline constexpr size_t kTextureHeightAt = 12;
		inline constexpr size_t kTextureFormatAt = 16;
		inline constexpr size_t kTextureDdsAt = 28;
		inline constexpr size_t kDdsHeader = 128;
		inline constexpr uint32_t kFormatBgra = 21;
		inline constexpr int kVsSide = 1024;
		inline constexpr int kMenuSide = 2048;
	}

	namespace Music
	{
		inline constexpr const char* kLoaderAnchor = "BGM_%03d";
		inline constexpr const char* kExtensionAnchor = ".ogg";
		inline constexpr const char* kSetNative = "BGM_Set";
		inline constexpr const char* kSetNumberNative = "BGM_SetNum";

		inline constexpr uint8_t kClearTable[] = { 0x68, 0x00, 0x32, 0x00, 0x00, 0x6A, 0x00, 0x68 };
		inline constexpr size_t kClearTableValueAt = 8;
		inline constexpr uint8_t kJumpNear = 0xE9;
		inline constexpr size_t kJumpNearLength = 5;
		inline constexpr uint8_t kStoreEax = 0xA3;
		inline constexpr uint8_t kLoadEdx[] = { 0x8B, 0x15 };
		inline constexpr uint8_t kLoadEcx[] = { 0x8B, 0x0D };
		inline constexpr uint8_t kStoreConstant[] = { 0xC7, 0x05 };
		inline constexpr uint8_t kCompareGlobal[] = { 0x83, 0x3D };
		inline constexpr size_t kOperandAt = 2;
		inline constexpr size_t kConstantAt = 6;
		inline constexpr uint8_t kCompareZeroAt = 6;
		inline constexpr uint32_t kNoTrack = 0xFFFFFFFF;
		inline constexpr size_t kLeastSetCalls = 2;

		inline constexpr int kSlotCount = 200;
		inline constexpr uintptr_t kSlotBytes = 0x40;
		inline constexpr uintptr_t kSlotRoot = 0x00;
		inline constexpr uintptr_t kSlotPresent = 0x04;
		inline constexpr uintptr_t kSlotLoop = 0x08;
		inline constexpr uintptr_t kSlotLoopPosition = 0x10;
		inline constexpr uintptr_t kSlotVolume = 0x18;
		inline constexpr uintptr_t kSlotFile = 0x20;
		inline constexpr size_t kSlotFileBytes = 32;
		inline constexpr int kFullVolume = 10000;
	}
}
