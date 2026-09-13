#include "Training/Camera.h"

#include "Core/interfaces.h"
#include "Core/utils.h"
#include "Game/GameOffsets.h"
#include "Training/BattleMap.h"

#include <cstdint>

namespace {

namespace CameraOffsets = GameOffsets::Camera;

constexpr float kMostZoom = 100.0f;

const char* const kElementNames[CameraOffsets::kElementCount] = { "Display", "True", "View" };

}

bool Camera::IsAvailable()
{
	return BattleMap::Addresses().camera != 0;
}

bool Camera::Read(View& out)
{
	return ReadElement(g_settings.hitboxCameraElement, out);
}

bool Camera::ReadElement(int element, View& out)
{
	const uintptr_t camera = BattleMap::Addresses().camera;

	if (camera == 0 || element < 0 || element >= CameraOffsets::kElementCount)
		return false;

	const uintptr_t base = camera + static_cast<uintptr_t>(element) * CameraOffsets::kElementBytes;

	int32_t x = 0;
	int32_t y = 0;
	float zoom = 0.0f;

	if (!TryRead(base + CameraOffsets::kElementX, x) || !TryRead(base + CameraOffsets::kElementY, y) ||
		!TryRead(base + CameraOffsets::kElementZoom, zoom))
	{
		return false;
	}

	if (!(zoom > 0.0f) || zoom > kMostZoom)
		return false;

	out.x = static_cast<float>(x);
	out.y = static_cast<float>(y);
	out.zoom = zoom;
	return true;
}

void Camera::ToScreen(const View& view, float worldX, float worldY, float width, float height, float& outX, float& outY)
{
	const float referenceX = CameraOffsets::kReferenceOriginX + (worldX - view.x) * view.zoom / CameraOffsets::kSubpixels;
	const float referenceY = CameraOffsets::kReferenceOriginY + (worldY - view.y) * view.zoom / CameraOffsets::kSubpixels;

	outX = referenceX * width / CameraOffsets::kReferenceWidth;
	outY = referenceY * height / CameraOffsets::kReferenceHeight;
}

int Camera::ElementCount()
{
	return CameraOffsets::kElementCount;
}

const char* Camera::ElementName(int element)
{
	return element >= 0 && element < CameraOffsets::kElementCount ? kElementNames[element] : "";
}
