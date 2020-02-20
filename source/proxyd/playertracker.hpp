#pragma once

#include <mutex>
#include <unordered_map>

#include <boost/container/flat_set.hpp>

#include <common/coord.hpp>
#include <common/span.hpp>
#include <common/types.hpp>

namespace vitamine::proxyd
{
	template <typename T>
	class PlayerTracker
	{
		using SetType = boost::container::flat_set<T>;

		mutable std::mutex _mutex;
		std::unordered_map<ChunkCoord, SetType> _membership;
		std::unordered_map<ChunkCoord, SetType> _subscriptions;

		void subscribe(ChunkCoord regionStart, ChunkCoord regionEnd, T value)
		{
			using C = typename ChunkCoord::ComponentType;

			for(C x = regionStart.x; x <= regionEnd.x; ++x)
			for(C z = regionStart.z; z <= regionEnd.z; ++z)
			{
				auto& subs = _subscriptions[{x, z}];

				if(subs.find(value) == subs.end())
					subs.insert(value);
			}
		}

		void unsubscribe(ChunkCoord regionStart, ChunkCoord regionEnd, T value)
		{
			using C = typename ChunkCoord::ComponentType;

			for(C x = regionStart.x; x <= regionEnd.x; ++x)
			for(C z = regionStart.z; z <= regionEnd.z; ++z)
			{
				auto& subs = _subscriptions[{x, z}];

				auto it = subs.find(value);

				if(it != subs.end())
					subs.erase(it);
			}
		}

	public:
		std::unique_lock<std::mutex> lock()
		{
			return std::unique_lock(_mutex);
		}

		void enter(ChunkCoord coord, UInt8 viewDistance, T value)
		{
			_membership[coord].insert(value);
			subscribe(coord - ChunkCoord{viewDistance, viewDistance}, coord + ChunkCoord{viewDistance, viewDistance}, value);
		}

		void leave(ChunkCoord coord, UInt8 viewDistance, T value)
		{
			_membership[coord].erase(value);
			unsubscribe(coord - ChunkCoord{viewDistance, viewDistance}, coord + ChunkCoord{viewDistance, viewDistance}, value);
		}

		void move(ChunkCoord from, ChunkCoord to, T value)
		{
			_membership[from].erase(value);
			_membership[to].insert(value);
		}

		void subscribe(Span<ChunkCoord const> chunks, T value)
		{
			for(auto coord : chunks)
				_subscriptions[coord].insert(value);
		}

		void unsubscribe(Span<ChunkCoord const> chunks, T value)
		{
			for(auto coord : chunks)
				_subscriptions[coord].erase(value);
		}

		void updateViewDistance(ChunkCoord coord, UInt8 oldViewDistance, UInt8 newViewDistance, T value)
		{
			if(oldViewDistance == newViewDistance)
				return;

			using C = typename ChunkCoord::ComponentType;

			if(newViewDistance < oldViewDistance)
			{
				for(C dx = -oldViewDistance; dx <= oldViewDistance; ++dx)
				for(C dz = -oldViewDistance; dz <= oldViewDistance; ++dz)
				{
					if(dx <= newViewDistance && dz <= newViewDistance)
						continue;

					_subscriptions[coord + ChunkCoord{dx, dz}].erase(value);
				}
			}
			else
			{
				for(C dx = -newViewDistance; dx <= newViewDistance; ++dx)
				for(C dz = -newViewDistance; dz <= newViewDistance; ++dz)
				{
					if(dx <= oldViewDistance && dz <= oldViewDistance)
						continue;

					_subscriptions[coord + ChunkCoord{dx, dz}].insert(value);
				}
			}
		}

		SetType subscribers(ChunkCoord coord)
		{
			return _subscriptions[coord];
		}

		SetType members(ChunkCoord coord)
		{
			return _membership[coord];
		}
	};
}
