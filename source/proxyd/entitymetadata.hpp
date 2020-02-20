#pragma once

#include <stdexcept>
#include <variant>

#include <common/buffer.hpp>
#include <common/types.hpp>
#include <proxyd/deserialize.hpp>
#include <proxyd/serialize.hpp>

namespace vitamine::proxyd
{
	enum struct EntityMetadataType : Int32
	{
		BYTE,
		VARINT,
		FLOAT,
		STRING,
		CHAT,
		OPT_CHAT,
		SLOT,
		BOOL,
		ROTATION,
		POSITION,
		OPT_POSITION,
		DIRECTION,
		OPT_UUID,
		OPT_BLOCK_ID,
		NBT,
		PARTICLE,
		VILLAGER_DATA,
		OPT_VARINT,
		POSE,
	};

	enum struct EntityMetadataPose : Int32
	{
		STANDING = 0,
		FALL_FLYING = 1,
		SLEEPING = 2,
		SWIMMING = 3,
		SPIN_ATTACK = 4,
		CROUCHING = 5,
		DYING = 6,
	};

	struct EntityMetadata
	{
		UInt8 index;
		EntityMetadataType type;
		std::variant<UInt8, Int32, Float32, Span<Char8 const>, bool, EntityMetadataPose> value;
	};

	inline
	void serializeEntityMetadata(Buffer& buffer, std::vector<EntityMetadata> const& meta)
	{
		for(auto& entry : meta)
		{
			serializeInt(buffer, entry.index);
			serializeVarInt(buffer, (Int32)entry.type);

			switch(entry.type)
			{
			case EntityMetadataType::BYTE:
				serializeInt(buffer, std::get<UInt8>(entry.value));
				break;

			case EntityMetadataType::VARINT:
				serializeVarInt(buffer, std::get<Int32>(entry.value));
				break;

			case EntityMetadataType::FLOAT:
				serializeFloat(buffer, std::get<Float32>(entry.value));
				break;

			case EntityMetadataType::BOOL:
				serializeBool(buffer, std::get<bool>(entry.value));
				break;

			case EntityMetadataType::POSE:
				serializeVarInt(buffer, (Int32)std::get<EntityMetadataPose>(entry.value));
				break;

			default:
				// TODO: implement
				throw std::runtime_error("unimplemented");
			}
		}

		serializeInt(buffer, (UInt8)0xff);
	}

	inline
	DeserializeStatus deserializeEntityMetadata(UInt8 const** bufpp, UInt* sizep, std::vector<EntityMetadata>* out)
	{
		// TODO: implement
		throw std::runtime_error("unimplemented");
	}
}
