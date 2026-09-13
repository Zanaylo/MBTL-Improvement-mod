#pragma once

#include <cstddef>
#include <cstdint>

namespace GameOffsets
{
	namespace Files
	{
		inline constexpr const char* kReaderOpenAnchor = "./grpdat/Title/title00.pat";
		inline constexpr uint8_t kReaderOpenPrologue[] = { 0x55, 0x8B, 0xEC, 0xB8 };

		inline constexpr const wchar_t* kFileBufferAssert = L"m_pFileBuffer";
		inline constexpr const wchar_t* kFileSizeAssert = L"m_dwFileSize";
		inline constexpr size_t kMaxReaderFromMemoryLength = 0x200;
		inline constexpr uint8_t kReaderTypeOffset = 0x1C;
		inline constexpr uint32_t kReaderTypeMemory = 2;

		inline constexpr size_t kLoadCallWindow = 0x30;
		inline constexpr int kLeastLoadCallers = 50;

		inline constexpr uintptr_t kReaderData = 0x30;
		inline constexpr uintptr_t kReaderSize = 0x28;
		inline constexpr size_t kReaderBytes = 0x70;
		inline constexpr uint32_t kReaderTypeClosed = 5;
		inline constexpr int kOpenDecrypt = 1;
		inline constexpr int kOpenShare = 1;
		inline constexpr int kOpenFlags = 0;

		inline constexpr size_t kCloseCallIndex = 1;
		inline constexpr uint8_t kLoadEcxFromFrame[] = { 0x8B, 0x8D };
		inline constexpr size_t kLoadEcxFromFrameLength = 6;
		inline constexpr uint8_t kFarFrameLea[] = { 0x8D, 0x8D };
		inline constexpr size_t kFarFrameLeaLength = 6;
		inline constexpr uint8_t kNearFrameLea[] = { 0x8D, 0x4D };
		inline constexpr size_t kNearFrameLeaLength = 3;
		inline constexpr uint8_t kCtorPrologue[] = { 0x55, 0x8B, 0xEC, 0x51, 0x89, 0x4D, 0xFC };
		inline constexpr int kLeastCtorVotes = 50;
		inline constexpr size_t kDtorMaxLength = 0x40;
		inline constexpr size_t kLeastDtorCallers = 100;

		inline constexpr const char* kObjectListAnchor = "%s/%s/object.txt";
		inline constexpr const char* kStageListAnchor = "./bg/BgList.txt";
		inline constexpr const char* kKernelLibrary = "KERNEL32.dll";
		inline constexpr const char* kFileAttributesImport = "GetFileAttributesA";
		inline constexpr uint8_t kFileExistsPrologue[] = { 0x55, 0x8B, 0xEC, 0x81, 0xEC };
	}

	namespace Battle
	{
		inline constexpr const char* kStepAnchor = "BattleProc Create";
		inline constexpr uint8_t kStepPrologue[] = { 0x55, 0x8B, 0xEC, 0x51, 0x8A, 0x45, 0x08, 0x88, 0x45, 0xFC, 0x8A,
			0x4D, 0x0C, 0x88, 0x4D, 0xFD, 0x8A, 0x55, 0x10, 0x88, 0x55, 0xFE };
		inline constexpr size_t kStepMaxLength = 0x60;

		inline constexpr const char* kTrainingNative = "IsTrainingBattle";
		inline constexpr uintptr_t kFrameCounter = 0x34;
		inline constexpr uintptr_t kMode = 0xC8;
		inline constexpr uintptr_t kSubMode = 0xCC;
		inline constexpr int kModeSingle = 0;
		inline constexpr int kModeTraining = 3;
		inline constexpr int kSubModeTraining = 1;

		inline constexpr const wchar_t* kSessionAnchor = L"p_session";
		inline constexpr size_t kSessionCompareWindow = 0x30;

		inline constexpr const char* kPauseAnchor = "PLAYER %d PAUSE";
		inline constexpr int kPauseLeastLoads = 20;
		inline constexpr uintptr_t kPauseState = 0x00;
	}

	namespace Objects
	{
		inline constexpr const char* kActivePlayerNative = "IsActivePlayer";
		inline constexpr uint32_t kLeastStride = 0x800;
		inline constexpr uint32_t kMostStride = 0x2000;
		inline constexpr size_t kStrideSearchWindow = 16;
		inline constexpr int kCharaSlots = 14;

		inline constexpr uintptr_t kActive = 0x864;
		inline constexpr uint8_t kEffectSpawnStore[] = { 0x64, 0x08, 0x00, 0x00, 0x01 };
		inline constexpr size_t kEffectListWindow = 24;
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
		inline constexpr int kStockLimit = 25;
		inline constexpr int kWideStockChara = 19;
		inline constexpr int kWideStockLimit = 33;
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

	namespace Scenes
	{
		inline constexpr const wchar_t* kStepAnchor = L"!\"Unknown scene!!!\"";
		inline constexpr uint8_t kOneArgumentCleanup[] = { 0x83, 0xC4, 0x04 };
		inline constexpr size_t kCallLength = 5;
		inline constexpr uint8_t kCall = 0xE8;

		inline constexpr const char* kReturnTitleAnchor = "ReturnTitle";
		inline constexpr size_t kRequestWindow = 0x30;
		inline constexpr uint8_t kPushByte = 0x6A;
		inline constexpr size_t kGetterCallAt = 2;
		inline constexpr size_t kMovEcxEaxAt = 7;
		inline constexpr uint8_t kMovEcxEax[] = { 0x8B, 0xC8 };
		inline constexpr size_t kRequestCallAt = 9;
		inline constexpr size_t kRequestSequenceLength = 14;
		inline constexpr uint16_t kRequestStackBytes = 4;

		inline constexpr uint8_t kEnteringSetterHead[] = { 0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x08, 0xA3 };
		inline constexpr size_t kEnteringSetterTailAt = 11;
		inline constexpr uint8_t kEnteringSetterTail[] = { 0x5D, 0xC3 };
		inline constexpr size_t kEnteringSetterLength = 13;

		inline constexpr size_t kLoadThisAt = 2;
		inline constexpr uint8_t kLoadThis[] = { 0x8B, 0x4D, 0xFC };
		inline constexpr size_t kSceneSetCallAt = 5;
		inline constexpr size_t kSceneSetSequenceLength = 10;
		inline constexpr size_t kSceneSetterWindow = 0x20;
		inline constexpr uint8_t kSceneStore[] = { 0x8B, 0x4D, 0x08, 0x89, 0x48, 0x08 };

		inline constexpr uint8_t kCountdownCompare[] = { 0x83, 0xB8 };
		inline constexpr size_t kCountdownOffsetAt = 2;
		inline constexpr size_t kCountdownLimitAt = 6;
		inline constexpr uint8_t kCountdownLimit = 0x1E;
		inline constexpr size_t kJumpAboveAt = 7;
		inline constexpr uint8_t kJumpAbove = 0x73;
		inline constexpr size_t kJumpShortAt = 9;
		inline constexpr uint8_t kJumpShort = 0xEB;
		inline constexpr size_t kLoadEcxAt = 11;
		inline constexpr uint8_t kLoadEcx = 0xB9;
		inline constexpr size_t kCountdownObjectAt = 12;
		inline constexpr size_t kCountdownCallAt = 16;
		inline constexpr size_t kCountdownSequenceLength = 17;

		inline constexpr uintptr_t kSceneId = 0x08;
	}

	namespace Draw
	{
		inline constexpr uint8_t kTwoArgumentCleanup[] = { 0x83, 0xC4, 0x08 };
		inline constexpr uint8_t kThreeArgumentCleanup[] = { 0x83, 0xC4, 0x0C };
		inline constexpr uint8_t kTypeTwoCompare[] = { 0xFF, 0xD2, 0x83, 0xF8, 0x02 };
		inline constexpr uint8_t kMovzx[] = { 0x0F, 0xB6 };
		inline constexpr size_t kActiveDisplacementAt = 3;
		inline constexpr uint8_t kActiveDisplacement[] = { 0x64, 0x08, 0x00, 0x00 };
		inline constexpr uint8_t kDrawFlagReturn[] = { 0x0F, 0xB6, 0x4D, 0x0C, 0x85, 0xC9, 0x75, 0x0A, 0xB8, 0x01, 0x00,
			0x00, 0x00 };
		inline constexpr size_t kTypeSlot = 2;
		inline constexpr int kCharacterType = 1;
		inline constexpr int kEffectType = 2;
	}

	namespace Bloom
	{
		inline constexpr const char* kBrightnessName = "g_Blightness";
		inline constexpr size_t kCallLength = 5;
		inline constexpr uint8_t kFiveArgumentCleanup[] = { 0x83, 0xC4, 0x14 };
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
		inline constexpr const char* kBankAnchor = "bank Over MAX_BG_CHARA_COLOR_INFO_NUM!!!";
		inline constexpr int kLeastBuilderCalls = 3;
		inline constexpr uint8_t kFourArgumentCleanup[] = { 0x83, 0xC4, 0x10 };
		inline constexpr uint8_t kThreeArgumentCleanup[] = { 0x83, 0xC4, 0x0C };
		inline constexpr size_t kPairGetters = 2;
		inline constexpr size_t kSingleGetters = 1;
		inline constexpr int kColourChannels = 3;
		inline constexpr int kFullPercent = 100;
		inline constexpr float kWhite = 1.0f;
		inline constexpr float kNone = 0.0f;
	}

	namespace Cockpit
	{
		inline constexpr const char* kViewNative = "Cockpit_SetView";
		inline constexpr uint8_t kSetterStore[] = { 0x8B, 0x4D, 0x08, 0x89, 0x48 };
		inline constexpr size_t kSetterWindow = 0x20;
		inline constexpr uint32_t kViewShown = 0;
		inline constexpr uint32_t kViewHidden = 1;
	}

	namespace Roster
	{
		inline constexpr const char* kTableAnchor = "./System/BtlCharaTbl.txt";
		inline constexpr const char* kNetworkNative = "GetMvNetworkInfo";
		inline constexpr uint8_t kCall = 0xE8;
		inline constexpr uint8_t kLoadEcx = 0xB9;
		inline constexpr size_t kGetterWindow = 0x40;
		inline constexpr uint8_t kAddEcxNetwork[] = { 0x81, 0xC1, 0xC8, 0x02, 0x00, 0x00 };
		inline constexpr uintptr_t kNetworkBase = 0x2C8;
		inline constexpr uint8_t kByteLoad[] = { 0x8A, 0x80 };
		inline constexpr size_t kShortFunction = 0x20;

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

	namespace Render
	{
		inline constexpr int kSceneWidth = 1280;
		inline constexpr int kSceneHeight = 720;

		inline constexpr const char* kStageDrawAnchor = "TinyFXAA";
		inline constexpr const char* kStageSpecularAnchor = "TecBgSpeculer";
		inline constexpr uint8_t kGlobalCompare[] = { 0x83, 0x3D };
		inline constexpr size_t kGlobalCompareLength = 7;
		inline constexpr int kGateCompares = 3;
		inline constexpr uint8_t kReturnTrue[] = { 0xB0, 0x01, 0x5D, 0xC3 };
		inline constexpr size_t kGateMaxLength = 0x40;

		inline constexpr uint8_t kByteGetterHead[] = { 0x55, 0x8B, 0xEC, 0xA0 };
		inline constexpr size_t kByteGetterTailAt = 8;
		inline constexpr uint8_t kByteGetterTail[] = { 0x5D, 0xC3 };
		inline constexpr size_t kByteGetterLength = 10;
		inline constexpr uint8_t kStretchRectCall[] = { 0x8B, 0x82, 0x88, 0x00, 0x00, 0x00, 0xFF, 0xD0 };
	}

	namespace Input
	{
		inline constexpr const char* kStickNative = "GetStickHold";
		inline constexpr uint8_t kAddEcx[] = { 0x81, 0xC1 };
		inline constexpr size_t kAddEcxLength = 6;
		inline constexpr uint8_t kLeverGetterPrologue[] = { 0x55, 0x8B, 0xEC, 0x51, 0x89, 0x4D, 0xFC, 0x8B, 0x45, 0xFC,
			0x8B, 0x00, 0x25, 0x00, 0x00, 0x00, 0xFF, 0xC1, 0xE8, 0x18 };
		inline constexpr int kLeverShift = 24;
		inline constexpr uint32_t kButtonMask = 0x00FFFFFF;
		inline constexpr const char* kXInputLibrary = "xinput1_3.dll";
		inline constexpr int kXInputGetStateOrdinal = 2;
	}

	namespace Meter
	{
		inline constexpr const char* kTeamNative = "IsActivePlayer";
		inline constexpr uint8_t kTeamIndexerHead[] = { 0x55, 0x8B, 0xEC, 0x69, 0x45, 0x08 };
		inline constexpr size_t kTeamIndexerStrideAt = 6;
		inline constexpr size_t kTeamIndexerAddAt = 10;
		inline constexpr uint8_t kTeamIndexerAdd = 0x05;
		inline constexpr size_t kTeamIndexerBaseAt = 11;
		inline constexpr size_t kTeamIndexerTailAt = 15;
		inline constexpr uint8_t kTeamIndexerTail[] = { 0x5D, 0xC3 };
		inline constexpr size_t kTeamIndexerLength = 17;
		inline constexpr uintptr_t kTeamSlot = 0x00;
		inline constexpr uintptr_t kTeamPoint = 0x14;

		inline constexpr const char* kComboNatives[] = { "GetComboInfo", "AddComboCount", "ComboView_Set" };
		inline constexpr uint8_t kComboStride[] = { 0x69, 0x45, 0x08 };
		inline constexpr uint8_t kComboOffset[] = { 0x8D, 0x84, 0x01 };
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

	namespace Music
	{
		inline constexpr const char* kLoaderAnchor = "BGM_%03d";
		inline constexpr const wchar_t* kPathBuilderAssert = L"g_BgmInfo[bgm_no].BgmType < BGM_INFO::eBgmType_Max";
		inline constexpr const char* kExtensionAnchor = ".ogg";
		inline constexpr const char* kSetNative = "BGM_Set";
		inline constexpr const char* kSetNumberNative = "BGM_SetNum";

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
