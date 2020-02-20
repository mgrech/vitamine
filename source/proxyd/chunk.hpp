#pragma once

#include <memory>
#include <mutex>

#include <common/types.hpp>

namespace vitamine::proxyd
{
	struct ChunkSection
	{
		UInt16 blocks[16][16][16] = {};
	};

	struct Chunk
	{
		mutable std::mutex mutex;
		std::unique_ptr<ChunkSection> sections[16];
		Int32 biomes[16][16] = {};
		UInt16 heightmap[16][16] = {};
	};
}
