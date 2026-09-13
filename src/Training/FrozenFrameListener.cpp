#include "Training/FrozenFrameListener.h"

#include "Core/interfaces.h"
#include "D3D9/FrozenFrame.h"
#include "Training/FrameStepper.h"
#include "Training/GameState.h"

void FrozenFrameListener::OnDeviceReady(IDirect3DDevice9* device, const D3DPRESENT_PARAMETERS& parameters)
{
	FrozenFrame::OnDeviceReset(device, parameters.BackBufferWidth, parameters.BackBufferHeight,
		parameters.BackBufferFormat);
}

void FrozenFrameListener::OnDeviceLost()
{
	FrozenFrame::OnDeviceLost();
}

void FrozenFrameListener::OnPresent(IDirect3DDevice9* device)
{
	if (!g_settings.replayFrozenFrame)
		return;

	const bool stepped = FrameStepper::ConsumeSteppedFlag();

	if (FrameStepper::NeedsFrozenFrameReplay() && !stepped)
	{
		FrozenFrame::Draw(device);
		return;
	}

	if (GameState::AllowsTrainingTools())
		FrozenFrame::Capture(device);
}
