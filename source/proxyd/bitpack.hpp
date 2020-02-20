#pragma once

#include <cassert>
#include <cstring>

#include <common/types.hpp>

namespace vitamine
{
	inline
	void bitpack16to14(UInt16 const* in, UInt incount, UInt8* out)
	{
		assert(incount % 32 == 0);

		for(; incount >= 32; incount -= 32, in += 32)
		{
			UInt64 buf[7];

#define AT(n) ((UInt64)(in[n] & 0x3fffu))
			buf[0] =  AT( 0)         | (AT( 1) << 14u) | (AT( 2) << 28u) | (AT( 3) << 42u) | (AT( 4) << 56u);
			buf[1] = (AT( 4) >>  8u) | (AT( 5) <<  6u) | (AT( 6) << 20u) | (AT( 7) << 34u) | (AT( 8) << 48u) | (AT( 9) << 62u);
			buf[2] = (AT( 9) >>  2u) | (AT(10) << 12u) | (AT(11) << 26u) | (AT(12) << 40u) | (AT(13) << 54u);
			buf[3] = (AT(13) >> 10u) | (AT(14) <<  4u) | (AT(15) << 18u) | (AT(16) << 32u) | (AT(17) << 46u) | (AT(18) << 60u);
			buf[4] = (AT(18) >>  4u) | (AT(19) << 10u) | (AT(20) << 24u) | (AT(21) << 38u) | (AT(22) << 52u);
			buf[5] = (AT(22) >> 12u) | (AT(23) <<  2u) | (AT(24) << 16u) | (AT(25) << 30u) | (AT(26) << 44u) | (AT(27) << 58u);
			buf[6] = (AT(27) >>  6u) | (AT(28) <<  8u) | (AT(29) << 22u) | (AT(30) << 36u) | (AT(31) << 50u);
#undef AT

			std::memcpy(out, buf, sizeof buf);
			out += sizeof buf;
		}
	}

	inline
	void bitpack16to9(UInt16 const* in, UInt incount, UInt8* out)
	{
		assert(incount % 8 == 0);

		for(; incount >= 8; incount -= 8, in += 8)
		{
#define AT(n) ((UInt64)in[n])
			UInt64 lower = AT(0) | (AT(1) << 9u) | (AT(2) << 18u) | (AT(3) << 27u) | (AT(4) << 36u) | (AT(5) << 45u) | (AT(6) << 54u) | (AT(7) << 63u);
			UInt8 upper = AT(7) >> 1u;
#undef AT

			std::memcpy(out, &lower, sizeof lower);
			out += sizeof lower;
			*out++ = upper;
		}
	}
}
