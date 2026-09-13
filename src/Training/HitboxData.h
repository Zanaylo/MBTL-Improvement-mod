#pragma once

#include <cstdint>

namespace HitboxData
{
	constexpr int kMaxObjects = 128;
	constexpr int kMaxBoxes = 96;

	enum BoxKind
	{
		BoxKind_Push,
		BoxKind_Hurt,
		BoxKind_Attack,
		BoxKind_Clash,
		BoxKind_Special,
	};

	struct Box
	{
		int16_t x1;
		int16_t y1;
		int16_t x2;
		int16_t y2;
		int index;
		BoxKind kind;
		bool inactive;
	};

	struct Object
	{
		uintptr_t address;
		uintptr_t frame;
		int x;
		int y;
		uint32_t flags;
		bool facingLeft;
		bool effect;
		int slot;
	};

	bool IsAvailable();

	int Objects(Object* out, int capacity);
	int Boxes(const Object& object, Box* out, int capacity);
}
