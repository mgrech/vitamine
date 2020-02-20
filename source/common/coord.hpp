#pragma once

#include <common/bits.hpp>
#include <common/constants.hpp>
#include <common/types.hpp>
#include <common/vector.hpp>

namespace vitamine
{
	using BlockCoord      = Vector3XYZ<Int32,   struct BlockCoordTag>;
	using ChunkCoord      = Vector2XZ <Int32,   struct ChunkCoordTag>;
	using ChunkBlockCoord = Vector3XYZ<Int32,   struct ChunkBlockCoordTag>;
	using EntityCoord     = Vector3XYZ<Float64, struct EntityCoordTag>;

	namespace detail
	{
		template <typename, typename>
		struct CoordCaster;

		template <typename TargetType, typename SourceType>
		constexpr TargetType coord_cast(SourceType value)
		{
			return CoordCaster<SourceType, TargetType>::cast(value);
		}

		template <typename T>
		struct CoordCaster<T, T>
		{
			static
			constexpr T cast(T coord)
			{
				return coord;
			}
		};

		template <>
		struct CoordCaster<BlockCoord, ChunkCoord>
		{
			static
			constexpr ChunkCoord cast(BlockCoord coord)
			{
				return {ashr(coord.x, CHUNK_BLOCKS_XZ_BITS), ashr(coord.z, CHUNK_BLOCKS_XZ_BITS)};
			}
		};

		template <>
		struct CoordCaster<ChunkCoord, BlockCoord>
		{
			static
			constexpr BlockCoord cast(ChunkCoord coord)
			{
				return {coord.x << CHUNK_BLOCKS_XZ_BITS, 0, coord.z << CHUNK_BLOCKS_XZ_BITS};
			}
		};

		template <>
		struct CoordCaster<BlockCoord, ChunkBlockCoord>
		{
			static
			constexpr ChunkBlockCoord cast(BlockCoord coord)
			{
				return {coord.x & 0xF, coord.y, coord.z & 0xF};
			}
		};

		template <>
		struct CoordCaster<BlockCoord, EntityCoord>
		{
			static
			constexpr EntityCoord cast(BlockCoord coord)
			{
				return {(Float64)coord.x, (Float64)coord.y, (Float64)coord.z};
			}
		};

		template <>
		struct CoordCaster<EntityCoord, BlockCoord>
		{
			static
			constexpr BlockCoord cast(EntityCoord coord)
			{
				using T = typename BlockCoord::ComponentType;
				return {(T)coord.x, (T)coord.y, (T)coord.z};
			}
		};

		template <>
		struct CoordCaster<ChunkCoord, EntityCoord>
		{
			static
			constexpr EntityCoord cast(ChunkCoord coord)
			{
				return coord_cast<EntityCoord>(coord_cast<BlockCoord>(coord));
			}
		};

		template <>
		struct CoordCaster<EntityCoord, ChunkCoord>
		{
			static
			constexpr ChunkCoord cast(EntityCoord coord)
			{
				return coord_cast<ChunkCoord>(coord_cast<BlockCoord>(coord));
			}
		};

		template <>
		struct CoordCaster<EntityCoord, ChunkBlockCoord>
		{
			static
			constexpr ChunkBlockCoord cast(EntityCoord coord)
			{
				return coord_cast<ChunkBlockCoord>(coord_cast<BlockCoord>(coord));
			}
		};
	}

	template <typename TargetType, typename SourceType>
	constexpr TargetType coord_cast(SourceType value)
	{
		return detail::coord_cast<TargetType>(value);
	}
}
