#include "Music/MusicModule.h"

#include "D3D9/DeviceHooks.h"
#include "Game/ModFiles.h"
#include "Music/BgmControl.h"
#include "Music/BgmRules.h"
#include "Music/BgmShuffle.h"
#include "Music/BgmTextOverlay.h"
#include "Music/BgmVolume.h"
#include "Music/MusicAnchors.h"
#include "Music/MusicDebugSection.h"
#include "Music/UserTracks.h"

namespace {

class MusicFrameListener : public IDeviceListener
{
public:
	void OnPresent(IDirect3DDevice9*) override { BgmControl::OnPresent(); }
};

MusicFrameListener g_listener;
BgmTextOverlay g_overlay;
MusicDebugSection g_debug;

}

void MusicModule::Install()
{
	MusicAnchors::Resolve();

	BgmVolume::Load();
	BgmRules::Load();
	BgmShuffle::Load();
	UserTracks::Load();

	ModFiles::AddOverlay(&g_overlay);
	DeviceHooks::AddListener(&g_listener);
	DebugSections::Add(&g_debug);

	const MusicAddresses& addresses = MusicAnchors::Get();

	BgmVolume::Install(addresses.setVolume);
	BgmControl::Install(addresses.play, addresses.stop, addresses.start);
}
