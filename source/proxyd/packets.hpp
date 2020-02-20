#pragma once

#include <utility>
#include <variant>

#include <boost/container/small_vector.hpp>
#include <boost/uuid/uuid.hpp>

#include <proxyd/deserialize.hpp>
#include <proxyd/entitymetadata.hpp>
#include <proxyd/nbt.hpp>
#include <proxyd/serialize.hpp>

namespace vitamine::proxyd
{
	using PacketId = Int32;

#define PACKET_BEGIN(name, id) \
	struct Packet##name \
	{ \
		static constexpr PacketId ID = id;

#define PACKET_FIELD_BOOL(name)               bool name;
#define PACKET_FIELD_INT(name, bits)          Int##bits name;
#define PACKET_FIELD_UINT(name, bits)         UInt##bits name;
#define PACKET_FIELD_FLOAT(name, bits)        Float##bits name;
#define PACKET_FIELD_VARINT(name, bits)       Int##bits name;
#define PACKET_FIELD_STRING(name)             Span<Char8 const> name;
#define PACKET_FIELD_IMPLICIT_TAILBYTES(name) Span<UInt8 const> name;
#define PACKET_FIELD_NBT(name)                Nbt name;
#define PACKET_FIELD_UUID(name)               boost::uuids::uuid name;
#define PACKET_FIELD_VARBYTES(name)           Span<UInt8 const> name;
#define PACKET_FIELD_ENTITY_METADATA(name)    std::vector<EntityMetadata> name;
#define PACKET_FIELD_ARRAY(name, ...)         struct name##_fields { __VA_ARGS__ }; boost::container::small_vector<name##_fields, 4 * sizeof(name##_fields)> name;

#define PACKET_END() \
	};

#include <proxyd/packets.def>

#define PACKET_BEGIN(name, id) \
	inline \
	bool deserializePacketPayload(Span<UInt8 const> buffer, Packet##name* out) \
	{ \
		using Packet = Packet##name; \
		(void)Packet{}; \
		auto bufp = buffer.data(); \
		auto size = buffer.size();

#define CHECK_DESERIALIZE(val) \
		if(val != DeserializeStatus::OK) \
			return false;

#define PACKET_FIELD_BOOL(name)               CHECK_DESERIALIZE(deserializeBool(&bufp, &size, &out->name))
#define PACKET_FIELD_INT(name, bits)          CHECK_DESERIALIZE(deserializeInt(&bufp, &size, &out->name))
#define PACKET_FIELD_UINT(name, bits)         CHECK_DESERIALIZE(deserializeInt(&bufp, &size, &out->name))
#define PACKET_FIELD_FLOAT(name, bits)        CHECK_DESERIALIZE(deserializeFloat(&bufp, &size, &out->name))
#define PACKET_FIELD_VARINT(name, bits)       CHECK_DESERIALIZE(deserializeVarInt(&bufp, &size, &out->name))
#define PACKET_FIELD_STRING(name)             CHECK_DESERIALIZE(deserializeString(&bufp, &size, &out->name))
#define PACKET_FIELD_IMPLICIT_TAILBYTES(name) CHECK_DESERIALIZE(deserializeImplicitTailBytes(&bufp, &size, &out->name))
#define PACKET_FIELD_NBT(name)                CHECK_DESERIALIZE(deserializeNbt(&bufp, &size, &out->name))
#define PACKET_FIELD_UUID(name)               CHECK_DESERIALIZE(deserializeUuid(&bufp, &size, &out->name))
#define PACKET_FIELD_ENTITY_METADATA(name)    CHECK_DESERIALIZE(deserializeEntityMetadata(&bufp, &size, &out->name))

#define PACKET_FIELD_VARBYTES(name) Int32 name##length; \
                                    CHECK_DESERIALIZE(deserializeVarInt(&bufp, &size, &name##length)) \
                                    UInt8 const* name##ptr; \
                                    CHECK_DESERIALIZE(deserializeBytes(&bufp, &size, name##length, &name##ptr)) \
                                    out->name = Span(name##ptr, name##length);

#define PACKET_FIELD_ARRAY(name, ...) Int32 name##length; \
                                      CHECK_DESERIALIZE(deserializeVarInt(&bufp, &size, &name##length)) \
                                      while(name##length--) \
                                      { \
	                                      Packet::name##_fields fields; \
	                                      { \
		                                      auto* out = &fields; \
		                                      __VA_ARGS__ \
	                                      } \
	                                      out->name.push_back(std::move(fields)); \
                                      }

#define PACKET_END() \
		return size == 0; \
	}

#include <proxyd/packets.def>
#undef CHECK_DESERIALIZE

#define PACKET_BEGIN(name, id) \
	inline \
	void serializePacketPayload(Buffer& buffer, Packet##name const& packet) \
	{

#define PACKET_FIELD_BOOL(name)               serializeBool(buffer, packet.name);
#define PACKET_FIELD_INT(name, bits)          serializeInt(buffer, packet.name);
#define PACKET_FIELD_UINT(name, bits)         serializeInt(buffer, packet.name);
#define PACKET_FIELD_FLOAT(name, bits)        serializeFloat(buffer, packet.name);
#define PACKET_FIELD_VARINT(name, bits)       serializeVarInt(buffer, packet.name);
#define PACKET_FIELD_STRING(name)             serializeString(buffer, packet.name);
#define PACKET_FIELD_IMPLICIT_TAILBYTES(name) serializeBytes(buffer, packet.name);
#define PACKET_FIELD_NBT(name)                serializeNbt(buffer, packet.name);
#define PACKET_FIELD_UUID(name)               serializeUuid(buffer, packet.name);
#define PACKET_FIELD_VARBYTES(name)           serializeVarInt(buffer, (Int32)packet.name.size()); \
                                              serializeBytes(buffer, packet.name);

#define PACKET_FIELD_ENTITY_METADATA(name) serializeEntityMetadata(buffer, packet.name);

#define PACKET_FIELD_ARRAY(name, ...) serializeVarInt(buffer, (Int32)packet.name.size()); \
                                      for(auto& packet : packet.name) \
	                                      __VA_ARGS__

#define PACKET_END() \
	}

#include <proxyd/packets.def>

	struct PacketPlayerInfo
	{
		static constexpr PacketId ID = 0x33;

		enum Action : Int32
		{
			ADD_PLAYER = 0,
			UPDATE_GAMEMODE = 1,
			UPDATE_LATENCY = 2,
			UPDATE_DISPLAYNAME = 3,
			REMOVE_PLAYER = 4,
		};

		struct AddPlayer
		{
			struct Property
			{
				std::string name;
				std::string value;
				bool isSigned;
				std::string signature;
			};

			std::string name;
			std::vector<Property> properties;
			Int32 gameMode;
			Int32 ping;
			bool hasDisplayName;
			std::string displayName;
		};

		struct UpdateGameMode
		{
			Int32 gameMode;
		};

		struct UpdateLatency
		{
			Int32 ping;
		};

		struct UpdateDisplayName
		{
			bool hasDisplayName;
			std::string displayName;
		};

		struct RemovePlayer
		{
		};

		struct Entry
		{
			boost::uuids::uuid uuid;
			std::variant<AddPlayer, UpdateGameMode, UpdateLatency, UpdateDisplayName, RemovePlayer> update;
		};

		Action action;
		std::vector<Entry> entries;
	};

	inline
	void serializePacketPayload(Buffer& buffer, PacketPlayerInfo const& packet)
	{
		serializeVarInt(buffer, (Int32)packet.action);
		serializeVarInt(buffer, (Int32)packet.entries.size());

		for(auto& entry : packet.entries)
		{
			serializeUuid(buffer, entry.uuid);

			switch(packet.action)
			{
			case PacketPlayerInfo::ADD_PLAYER:
			{
				auto& update = std::get<PacketPlayerInfo::AddPlayer>(entry.update);
				serializeString(buffer, update.name);
				serializeVarInt(buffer, (Int32)update.properties.size());

				for(auto& prop : update.properties)
				{
					serializeString(buffer, prop.name);
					serializeString(buffer, prop.value);
					serializeBool(buffer, prop.isSigned);

					if(prop.isSigned)
						serializeString(buffer, prop.signature);
				}

				serializeVarInt(buffer, update.gameMode);
				serializeVarInt(buffer, update.ping);
				serializeBool(buffer, update.hasDisplayName);

				if(update.hasDisplayName)
					serializeString(buffer, update.displayName);

				break;
			}

			case PacketPlayerInfo::UPDATE_GAMEMODE:
			{
				auto update = std::get<PacketPlayerInfo::UpdateGameMode>(entry.update);
				serializeVarInt(buffer, update.gameMode);
				break;
			}
			case PacketPlayerInfo::UPDATE_LATENCY:
			{
				auto update = std::get<PacketPlayerInfo::UpdateLatency>(entry.update);
				serializeVarInt(buffer, update.ping);
				break;
			}
			case PacketPlayerInfo::UPDATE_DISPLAYNAME:
			{
				auto update = std::get<PacketPlayerInfo::UpdateDisplayName>(entry.update);
				serializeBool(buffer, update.hasDisplayName);

				if(update.hasDisplayName)
					serializeString(buffer, update.displayName);

				break;
			}
			case PacketPlayerInfo::REMOVE_PLAYER:
				break;
			}
		}
	}
}
