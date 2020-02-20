#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <boost/container/flat_set.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <common/buffer.hpp>
#include <common/clock.hpp>
#include <common/coord.hpp>
#include <common/span.hpp>
#include <common/types.hpp>
#include <common/vector.hpp>
#include <common/net/connection.hpp>
#include <proxyd/chat.hpp>
#include <proxyd/deserialize.hpp>
#include <proxyd/framing.hpp>
#include <proxyd/globalstate.hpp>
#include <proxyd/packetreader.hpp>
#include <proxyd/packets.hpp>
#include <proxyd/playertracker.hpp>
#include <proxyd/serialize.hpp>
#include <proxyd/types.hpp>

namespace vitamine::proxyd
{
	enum struct ClientPhase
	{
		INITIAL,

		LOGIN,
		PLAY_INIT,
		PLAY,

		STATUS,
	};

	struct ClientSettings
	{
		std::string locale;
		UInt8 viewDistance;
		ChatMode chatMode;
		bool chatColorsEnabled;
		UInt8 displayedSkinParts;
		MainHand mainHand;
	};

	struct PlayerState
	{
		std::string clientBrand;
		std::string username;

		boost::uuids::uuid uuid = {};
		EntityId entityId;

		GameMode gameMode = GameMode::CREATIVE;
		ClientSettings clientSettings;

		EntityCoord position = {0, 64, 0};
		Float32 yaw = 0, pitch = 0;
		UInt8 heldItemSlot = 0;

		Int32 nextTeleportId = 0;
		boost::container::flat_set<Int32> outstandingTeleportIds;

		UInt8 openWindow = 0;

		UInt8 abilityFlags = 0x0f;
		Float32 walkingSpeed = 1.0;
		Float32 flyingSpeed = 1.0;

		bool sprinting = false;
		bool crouching = false;

		Int32 startTeleport()
		{
			auto id = nextTeleportId++;
			outstandingTeleportIds.insert(id);
			return id;
		}

		bool confirmTeleport(Int32 id)
		{
			auto it = outstandingTeleportIds.find(id);

			if(it == outstandingTeleportIds.end())
				return false;

			outstandingTeleportIds.erase(it);
			return true;
		}
	};

	class StateMachine
	{
		GlobalState* _globalState;

		std::shared_ptr<IConnection> _connection;
		PacketReader _reader;

		std::atomic<ClientPhase> _phase = ClientPhase::INITIAL;

		std::atomic<Int64> _lastPacketTime;
		std::atomic<Int64> _lastKeepAliveSentTime;

		PlayerState _playerState;

		void disconnect()
		{
			_connection->disconnect();
		}

		void disconnect(std::string const& message)
		{
			auto reason = chat::plainText(message);

			if(_phase == ClientPhase::PLAY)
			{
				PacketDisconnect packet;
				packet.reason = spanFromStdString(reason);
				sendPacket(packet);
			}
			else
			{
				PacketDisconnectLogin packet;
				packet.reason = spanFromStdString(reason);
				sendPacket(packet);
			}

			disconnect();
		}

		void sendPacket(Buffer const& buffer)
		{
			_connection->send(buffer);
		}

		template <typename Packet>
		void sendPacket(Packet const& packet)
		{
			sendPacket(serializePacket(packet));
		}

		void broadcastGloballyUnsafe(Buffer const& buffer, bool includeSelf)
		{
			for(auto& player : _globalState->players)
				if(includeSelf || player != this)
					player->_connection->send(buffer);
		}

		template <typename Packet>
		void broadcastGloballyUnsafe(Packet const& packet, bool includeSelf)
		{
			auto buffer = serializePacket(packet);
			broadcastGloballyUnsafe(buffer, includeSelf);
		}

		void broadcastLocallyUnsafe(Buffer const& buffer, bool includeSelf)
		{
			auto coord = coord_cast<ChunkCoord>(_playerState.position);

			for(auto& player : _globalState->playerTracker.subscribers(coord))
				if(includeSelf || player != this)
					player->_connection->send(buffer);
		}

		template <typename Packet>
		void broadcastLocallyUnsafe(Packet const& packet, bool includeSelf)
		{
			auto buffer = serializePacket(packet);
			broadcastLocallyUnsafe(buffer, includeSelf);
		}

		void broadcastLocally(Buffer const& buffer, bool includeSelf)
		{
			auto lock = _globalState->playerTracker.lock();
			broadcastLocallyUnsafe(buffer, includeSelf);
		}

		template <typename Packet>
		void broadcastLocally(Packet const& packet, bool includeSelf)
		{
			auto buffer = serializePacket(packet);
			broadcastLocally(buffer, includeSelf);
		}

		void sendChunk(ChunkCoord coord);

		void onPacket(PacketFrame frame);
		void onClientSettingsChange(PacketClientSettings const& packet);

		Buffer createMovePacket(EntityCoord oldPosition, bool rotate);

		static
		Buffer createSpawnPacket(PlayerState const& state);

		static
		Buffer createDespawnPacket(PlayerState const& state);

		void sendMetadataUpdate();

		void onMove(EntityCoord oldPosition, bool rotate);
		void onChunkTransition(ChunkCoord from, ChunkCoord to, EntityCoord oldPosition, bool rotate);

		Chunk* getOrCreateChunk(ChunkCoord coord);

	public:
		StateMachine(StateMachine const&) = delete;
		StateMachine& operator=(StateMachine const&) = delete;
		StateMachine(StateMachine&&) = delete;
		StateMachine& operator=(StateMachine&&) = delete;

		StateMachine(GlobalState* globalState, std::shared_ptr<IConnection> const& connection)
		: _globalState(globalState), _connection(connection)
		, _reader([this](auto frame){ onPacket(frame); }, [connection]{ connection->disconnect(); })
		, _lastPacketTime(_globalState->clock.now()), _lastKeepAliveSentTime(0)
		{}

		~StateMachine();

		void onTick();

		void onBytesReceived(Span<UInt8 const> data)
		{
			_reader.onBytesReceived(data);
		}
	};
}
