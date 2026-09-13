#pragma once

#include "Overlay/Debug/DebugSection.h"

#include <cstdint>

class HooksDebugSection : public IDebugSection
{
public:
	const char* Title() const override { return "Anchors and hooks"; }
	void Draw() override;
};

class DeviceDebugSection : public IDebugSection
{
public:
	const char* Title() const override { return "Device and frame"; }
	void Draw() override;
};

class MemoryDebugSection : public IDebugSection
{
public:
	const char* Title() const override { return "Memory viewer"; }
	void Draw() override;

private:
	void DrawRows();

	char m_address[16] = "00400000";
	int m_rows = 16;
	bool m_dwords = false;
};
