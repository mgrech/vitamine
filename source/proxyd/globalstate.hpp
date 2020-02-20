#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include <boost/uuid/uuid_generators.hpp>

#include <common/clockmonotonic.hpp>
#include <common/types.hpp>
#include <proxyd/chunk.hpp>
#include <proxyd/playertracker.hpp>
#include <proxyd/types.hpp>

namespace vitamine::proxyd
{
	class StateMachine;

	struct ServerSettings
	{
		Int8 maxViewDistance = 32;
		bool reducedDebugInfo = false;

		Dimension dimension = Dimension::OVERWORLD;
	};

	struct GlobalState
	{
		boost::uuids::name_generator uuidGenerator{boost::uuids::uuid{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0}};
		std::atomic<EntityId> nextEntityId{1};

		ServerSettings serverSettings;
		MonotonicClock clock;

		mutable std::mutex playersMutex;
		std::unordered_set<StateMachine*> players;

		PlayerTracker<StateMachine*> playerTracker;

		mutable std::mutex chunkMutex;
		std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>> chunks;
	};
}
