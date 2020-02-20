#pragma once

#include <type_traits>

#include <common/bits.hpp>
#include <common/types.hpp>

namespace vitamine::proxyd
{
	using EntityId = Int32;

	enum struct GameMode : UInt8
	{
		SURVIVAL,
		CREATIVE,
		ADVENTURE,
		SPECTATOR,

		HARDCORE = 1u << 3u,
	};

	enum struct ChatMode
	{
		ENABLED,
		COMMANDS_ONLY,
		HIDDEN,
	};

	enum struct MainHand
	{
		LEFT,
		RIGHT,
	};

	enum struct Dimension : Int32
	{
		NETHER    = -1,
		OVERWORLD =  0,
		END       =  1,
	};

	enum struct BlockFace : Int8
	{
		NEG_Y = 0,
		POS_Y = 1,
		NEG_Z = 2,
		POS_Z = 3,
		NEG_X = 4,
		POS_X = 5,
	};

	inline
	UInt64 toPosition(BlockCoord coord)
	{
		using T = BlockCoord::ComponentType;
		using U = std::make_unsigned_t<T>;

		auto xu = (UInt64)(U)coord.x;
		auto yu = (UInt64)(U)coord.y;
		auto zu = (UInt64)(U)coord.z;

		return ((xu & 0x3ff'ffffu) << 38u) | ((zu & 0x3ff'ffffu) << 12u) | (yu & 0xfffu);
	}

	inline
	BlockCoord fromPosition(UInt64 position)
	{
		using T = BlockCoord::ComponentType;

		auto x = ashr((Int64)position, 38);
		auto y = position & 0xfffu;
		auto z = ashr((Int64)position << 26, 38);

		return {(T)x, (T)y, (T)z};
	}
}
