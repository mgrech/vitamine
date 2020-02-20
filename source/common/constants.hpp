#pragma once

#include <common/bits.hpp>
#include <common/types.hpp>

namespace vitamine
{
	constexpr UInt CHUNK_BLOCKS_Y = 256;
	constexpr UInt CHUNK_BLOCKS_XZ = 16;
	constexpr UInt CHUNK_BLOCKS = CHUNK_BLOCKS_Y * CHUNK_BLOCKS_XZ * CHUNK_BLOCKS_XZ;

	constexpr UInt CHUNK_BLOCKS_Y_BITS = floorlog2(CHUNK_BLOCKS_Y);
	constexpr UInt CHUNK_BLOCKS_XZ_BITS = floorlog2(CHUNK_BLOCKS_XZ);
	constexpr UInt CHUNK_BLOCKS_BITS = floorlog2(CHUNK_BLOCKS);
}
