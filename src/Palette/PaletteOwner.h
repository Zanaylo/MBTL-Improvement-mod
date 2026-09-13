#pragma once

#include <cstdint>

namespace PaletteOwner
{
	constexpr int kSides = 2;
	constexpr int kMembers = 2;
	constexpr int kSeats = kSides * kMembers;

	struct Owner
	{
		uintptr_t address;
		uintptr_t shared;
		int chara;
		int colour;
	};

	constexpr int SeatOf(int side, int member) { return side + member * kSides; }
	constexpr int SideOf(int seat) { return seat % kSides; }
	constexpr int MemberOf(int seat) { return seat / kSides; }

	bool Read(int seat, Owner& out);
	int CharaNumber(int seat);
	bool HasPartner(int side);
}
