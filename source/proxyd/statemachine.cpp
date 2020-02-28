#include <proxyd/statemachine.hpp>

#include <unordered_set>

#include <proxyd/bitpack.hpp>
#include <proxyd/chunk.hpp>
#include <proxyd/packets.hpp>
#include <generated/ids.hpp>

namespace
{
	// https://wiki.vg/index.php?title=Protocol&oldid=15289
	constexpr vitamine::Int32 PROTOCOL_VERSION = 498;

	constexpr vitamine::Int64 CLIENT_READ_TIMEOUT_NANOS = 10'000'000'000;
	constexpr vitamine::Int64 KEEP_ALIVE_INTERVAL_NANOS = 5'000'000'000;

	constexpr vitamine::Char8 const PLUGIN_CHANNEL_MINECRAFT_BRAND[] = "minecraft:brand";
	constexpr vitamine::Char8 const SERVER_BRAND_STRING[] = "github.com/mgrech/vitamine";
}

namespace vitamine::proxyd
{
	void StateMachine::onPacket(PacketFrame frame)
	{
		_lastPacketTime = _globalState->clock.now();

		switch(_phase)
		{
		case ClientPhase::INITIAL:
			PacketHandshake handshake;
			if(frame.id != PacketHandshake::ID || !deserializePacketPayload(frame.data, &handshake))
			{
				disconnect();
				return;
			}

			if(handshake.version != PROTOCOL_VERSION)
			{
				disconnect("version mismatch");
				return;
			}

			switch(handshake.nextState)
			{
			case 1: _phase = ClientPhase::STATUS; break;
			case 2: _phase = ClientPhase::LOGIN; break;
			default:
				disconnect("invalid next state"); break;
			}

			break;

		case ClientPhase::LOGIN:
		{
			PacketLoginStart loginStart;
			if(frame.id != PacketLoginStart::ID || !deserializePacketPayload(frame.data, &loginStart))
			{
				disconnect();
				return;
			}

			std::printf("new player from %s\n", _connection->endpoint().c_str());

			// TODO: check for duplicate player names
			_playerState.username = toStdString(loginStart.name);
			_playerState.uuid = _globalState->uuidGenerator(_playerState.username);
			_playerState.entityId = _globalState->nextEntityId++;

			auto uuid = boost::uuids::to_string(_playerState.uuid);
			PacketLoginSuccess loginSuccess;
			loginSuccess.uuid = spanFromStdString(uuid);
			loginSuccess.username = loginStart.name;
			sendPacket(loginSuccess);

			PacketJoinGame joinGame;
			joinGame.entityId = _playerState.entityId;
			joinGame.gameMode = (UInt8)_playerState.gameMode;
			joinGame.dimension = (Int32)_globalState->serverSettings.dimension;
			joinGame.maxPlayers = 0;
			joinGame.levelType = spanFromCString("default");
			joinGame.viewDistance = _globalState->serverSettings.maxViewDistance;
			joinGame.reducedDebugInfo = _globalState->serverSettings.reducedDebugInfo;
			sendPacket(joinGame);

			Buffer buffer;
			serializeString(buffer, spanFromCString(SERVER_BRAND_STRING));

			PacketPluginMessageServer brandMessage;
			brandMessage.channel = spanFromCString(PLUGIN_CHANNEL_MINECRAFT_BRAND);
			brandMessage.data = Span((UInt8 const*)buffer.data(), buffer.size());
			sendPacket(brandMessage);

			PacketPlayerAbilitiesServer playerAbilities;
			playerAbilities.flags = _playerState.abilityFlags;
			playerAbilities.flyingSpeed = _playerState.flyingSpeed;
			playerAbilities.walkingSpeed = _playerState.walkingSpeed;
			sendPacket(playerAbilities);

			PacketHeldItemChangeServer heldItemChange;
			heldItemChange.slot = _playerState.heldItemSlot;
			sendPacket(heldItemChange);

			{
				std::lock_guard guard(_globalState->playersMutex);

				{
					std::vector<PacketPlayerInfo::Entry> entries;

					for(auto& player : _globalState->players)
					{
						PacketPlayerInfo::AddPlayer addPlayer;
						addPlayer.name = player->_playerState.username;
						addPlayer.gameMode = (Int32)player->_playerState.gameMode;
						addPlayer.ping = 0;
						addPlayer.hasDisplayName = false;

						PacketPlayerInfo::Entry entry;
						entry.uuid = player->_playerState.uuid;
						entry.update = addPlayer;
						entries.push_back(entry);
					}

					PacketPlayerInfo currentPlayers;
					currentPlayers.action = PacketPlayerInfo::ADD_PLAYER;
					currentPlayers.entries = entries;
					sendPacket(currentPlayers);
				}

				_globalState->players.insert(this);

				{
					PacketPlayerInfo::AddPlayer addPlayer;
					addPlayer.name = _playerState.username;
					addPlayer.gameMode = (Int32)_playerState.gameMode;
					addPlayer.ping = 0;
					addPlayer.hasDisplayName = false;

					PacketPlayerInfo::Entry entry;
					entry.uuid = _playerState.uuid;
					entry.update = addPlayer;

					PacketPlayerInfo addPlayerInfo;
					addPlayerInfo.action = PacketPlayerInfo::ADD_PLAYER;
					addPlayerInfo.entries.push_back(entry);
					broadcastGloballyUnsafe(addPlayerInfo, true);
				}
			}

			_phase = ClientPhase::PLAY_INIT;
			break;
		}

		case ClientPhase::PLAY_INIT:
			switch(frame.id)
			{
			case PacketPlayerPositionRotationClient::ID:
			{
				PacketPlayerPositionRotationClient packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				// discard this packet, the client is ready when receiving the teleport confirmation
				break;
			}

			case PacketTeleportConfirm::ID:
			{
				PacketTeleportConfirm packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				if(!_playerState.confirmTeleport(packet.teleportId))
					disconnect("PacketTeleportConfirm: invalid teleport id");

				auto coord = coord_cast<ChunkCoord>(_playerState.position);

				{
					auto vd = _playerState.clientSettings.viewDistance;
					auto spawn = createSpawnPacket(_playerState);

					std::unordered_set<StateMachine*> visiblePlayers;

					auto lock = _globalState->playerTracker.lock();
					_globalState->playerTracker.enter(coord, vd, this);
					broadcastLocallyUnsafe(spawn, false);

					for(auto i = -vd; i <= vd; ++i)
					for(auto j = -vd; j <= vd; ++j)
					{
						auto members = _globalState->playerTracker.members(coord + ChunkCoord{i, j});
						visiblePlayers.insert(members.begin(), members.end());
					}

					lock.unlock();

					for(auto member : visiblePlayers)
						if(member != this)
							sendPacket(createSpawnPacket(member->_playerState));
				}

				_phase = ClientPhase::PLAY;
				break;
			}

			case PacketPluginMessageClient::ID:
			{
				PacketPluginMessageClient packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				if(packet.channel == spanFromCString(PLUGIN_CHANNEL_MINECRAFT_BRAND))
				{
					Span<Char8 const> brand;
					auto bufp = packet.data.data();
					auto size = packet.data.size();

					if(deserializeString(&bufp, &size, &brand) != DeserializeStatus::OK || size != 0)
					{
						disconnect();
						return;
					}

					_playerState.clientBrand = std::string(brand.data(), brand.size());
				}
				else
					std::printf("PacketPluginMessageClient: unknown plugin message\n");

				break;
			}

			case PacketClientSettings::ID:
			{
				PacketClientSettings packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				onClientSettingsChange(packet);

				auto playerChunkCoord = coord_cast<ChunkCoord>(_playerState.position);
				auto cx = playerChunkCoord.x;
				auto cz = playerChunkCoord.z;
				auto vd = _playerState.clientSettings.viewDistance;

				for(auto i = cx - vd; i <= cx + vd; ++i)
					for(auto j = cz - vd; j <= cz + vd; ++j)
						sendChunk({i, j});

				PacketSpawnPosition spawnPosition;
				spawnPosition.location = toPosition({0, 0, 0});
				sendPacket(spawnPosition);

				PacketPlayerPositionLookServer positionLook;
				positionLook.x = _playerState.position.x;
				positionLook.y = _playerState.position.y;
				positionLook.z = _playerState.position.z;
				positionLook.yaw = _playerState.pitch;
				positionLook.pitch = _playerState.yaw;
				positionLook.flags = 0;
				positionLook.teleportId = _playerState.startTeleport();
				sendPacket(positionLook);
				break;
			}

			default:
				std::printf("unhandled packet in phase PLAY_INIT: ");
				debugPrint(frame);
				break;
			}
			break;

		case ClientPhase::PLAY:
			switch(frame.id)
			{
			case PacketChatClient::ID:
			{
				PacketChatClient packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				if(packet.message.size() > 256)
				{
					disconnect("chat message too long");
					return;
				}

				{
					auto message = chat::plainText("<" + _playerState.username + "> " + toStdString(packet.message));

					PacketChatServer serverChat;
					serverChat.position = 0;
					serverChat.chat = spanFromStdString(message);

					std::lock_guard guard(_globalState->playersMutex);
					broadcastGloballyUnsafe(serverChat, true);
				}

				break;
			}

			case PacketClientSettings::ID:
			{
				PacketClientSettings packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				onClientSettingsChange(packet);
				break;
			}

			case PacketCloseWindowClient::ID:
			{
				PacketCloseWindowClient packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				// ignore ID=0 (player inventory), since the client never sends an open packet for this id
				if(packet.windowId != 0)
				{
					if(_playerState.openWindow != packet.windowId)
					{
						disconnect("invalid window id");
						return;
					}

					_playerState.openWindow = 0;
				}

				break;
			}

			case PacketInteractEntity::ID:
			{
				PacketInteractEntity packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				// TODO: validate, but ignore
				break;
			}

			case PacketPlayerPosition::ID:
			{
				PacketPlayerPosition packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				auto oldPos = _playerState.position;
				_playerState.position.x = packet.x;
				_playerState.position.y = packet.y;
				_playerState.position.z = packet.z;
				onMove(oldPos, false);
				break;
			}

			case PacketPlayerPositionRotationClient::ID:
			{
				PacketPlayerPositionRotationClient packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				auto oldPos = _playerState.position;
				_playerState.position.x = packet.x;
				_playerState.position.y = packet.y;
				_playerState.position.z = packet.z;
				_playerState.yaw = packet.yaw;
				_playerState.pitch = packet.pitch;
				onMove(oldPos, true);
				break;
			}

			case PacketPlayerRotation::ID:
			{
				PacketPlayerRotation packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				_playerState.yaw = packet.yaw;
				_playerState.pitch = packet.pitch;

				PacketEntityRotation rotation;
				rotation.entityId = _playerState.entityId;
				rotation.yaw = _playerState.yaw * 256 / 360;
				rotation.pitch = _playerState.pitch * 256 / 360;
				rotation.onGround = false;

				PacketEntityHeadLook look;
				look.entityId = _playerState.entityId;
				look.headYaw = _playerState.yaw * 256 / 360;

				auto lock = _globalState->playerTracker.lock();
				broadcastLocallyUnsafe(rotation, false);
				broadcastLocallyUnsafe(look, false);
				break;
			}

			case PacketPlayerMovement::ID:
			{
				PacketPlayerMovement packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				// ignore
				break;
			}

			case PacketKeepAliveClient::ID:
			{
				PacketKeepAliveClient packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				// discard packet
				break;
			}

			case PacketPlayerAbilitiesClient::ID:
			{
				PacketPlayerAbilitiesClient packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				_playerState.abilityFlags = packet.flags;
				_playerState.flyingSpeed = packet.flyingSpeed;
				_playerState.walkingSpeed = packet.walkingSpeed;
				break;
			}

			case PacketPlayerDigging::ID:
			{
				PacketPlayerDigging packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				if(packet.status > 6)
				{
					disconnect("PacketPlayerDigging: invalid status");
					return;
				}

				if(packet.face > 5)
				{
					disconnect("PacketPlayerDigging: invalid block face");
					return;
				}

				auto location = fromPosition(packet.location);
				auto face = (BlockFace)packet.face;

				// TODO: validate if block face matches player direction
				(void)face;

				switch(packet.status)
				{
				case 0: // started digging
				{
					auto distance = (coord_cast<EntityCoord>(location) - _playerState.position).length();

					if(distance > 6)
					{
						// TODO: reject
						std::printf("player digging out of range\n");
					}

					std::unique_lock chunkListLock(_globalState->chunkMutex);
					auto it = _globalState->chunks.find(coord_cast<ChunkCoord>(location));

					if(it == _globalState->chunks.end())
					{
						std::printf("PacketPlayerDigging: chunk not found\n");
						disconnect();
						return;
					}

					auto& chunk = *it->second;
					chunkListLock.unlock();

					auto blockCoord = coord_cast<ChunkBlockCoord>(location);
					std::unique_lock chunkLock(chunk.mutex);

					if(!chunk.sections[blockCoord.y / 16])
						chunk.sections[blockCoord.y / 16] = std::make_unique<ChunkSection>();

					auto& block = chunk.sections[blockCoord.y / 16]->blocks[blockCoord.y % 16][blockCoord.z][blockCoord.x];

					// TODO: check for fluids
					// if the client thinks there is a block, but there is none (e.g. due to a race condition)
					if(block == BLOCKID_MINECRAFT_AIR)
						return;

					block = BLOCKID_MINECRAFT_AIR;
					chunkLock.unlock();

					PacketBlockChange blockChange;
					blockChange.location = packet.location;
					blockChange.blockId = BLOCKID_MINECRAFT_AIR;
					broadcastLocally(blockChange, false);

					break;
				}

				default:
					std::printf("unimplemented digging status: %d\n", packet.status);
					break;
				}

				break;
			}

			case PacketEntityAction::ID:
			{
				PacketEntityAction packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				switch(packet.actionId)
				{
				case 0: // start crouching
					if(packet.entityId != _playerState.entityId)
					{
						disconnect("EntityAction: invalid entity id for action 'start crouching'");
						return;
					}

					if(_playerState.crouching)
						disconnect("EntityAction: already crouching");
					else
					{
						_playerState.crouching = true;
						sendMetadataUpdate();
					}

					break;

				case 1: // stop crouching
					if(packet.entityId != _playerState.entityId)
					{
						disconnect("EntityAction: invalid entity id for action 'stop crouching'");
						return;
					}

					if(!_playerState.crouching)
						disconnect("EntityAction: not crouching");
					else
					{
						_playerState.crouching = false;
						sendMetadataUpdate();
					}

					break;

				case 2:
					std::printf("unimplemented EntityAction: leave bed\n");
					break;

				case 3: // start sprinting
					if(packet.entityId != _playerState.entityId)
					{
						disconnect("EntityAction: invalid entity id for action 'start sprinting'");
						return;
					}

					if(_playerState.sprinting)
						disconnect("EntityAction: already sprinting");
					else
					{
						_playerState.sprinting = true;
						sendMetadataUpdate();
					}

					break;

				case 4:
					if(packet.entityId != _playerState.entityId)
					{
						disconnect("EntityAction: invalid entity id for action 'stop sprinting'");
						return;
					}

					if(!_playerState.sprinting)
						disconnect("EntityAction: not sprinting");
					else
					{
						_playerState.sprinting = false;
						sendMetadataUpdate();
					}

					break;

				case 5:
					std::printf("unimplemented EntityAction: start jump with horse\n");
					break;

				case 6:
					std::printf("unimplemented EntityAction: stop jump with horse\n");
					break;

				case 7:
					std::printf("unimplemented EntityAction: open horse inventory\n");
					break;

				case 8:
					std::printf("unimplemented EntityAction: start flying with elytra\n");
					break;

				default:
					disconnect("EntityAction: invalid action id: " + std::to_string(packet.actionId));
					break;
				}

				break;
			}

			case PacketHeldItemChangeClient::ID:
			{
				PacketHeldItemChangeClient packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					break;
				}

				if(packet.slot < 0 || packet.slot > 8)
				{
					disconnect("HeldItemChange: invalid slot id");
					return;
				}

				_playerState.heldItemSlot = (UInt8)packet.slot;
				// TODO: relay to other clients
				break;
			}

			case PacketAnimationClient::ID:
			{
				PacketAnimationClient packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					break;
				}

				if(packet.hand != 0 && packet.hand != 1)
				{
					disconnect("Animation: invalid hand");
					break;
				}

				PacketEntityAnimation animation;
				animation.entityId = _playerState.entityId;
				animation.animationId = packet.hand == 0 ? 0 : 3;
				broadcastLocally(animation, false);
				break;
			}

			case PacketUseItem::ID:
			{
				PacketUseItem packet;

				if(!deserializePacketPayload(frame.data, &packet))
				{
					disconnect();
					return;
				}

				if(packet.hand != 0 && packet.hand != 1)
				{
					disconnect("PacketUseItem: invalid hand");
					return;
				}

				// TODO: implement
				break;
			}

			default:
				std::printf("unhandled packet in phase PLAY: ");
				debugPrint(frame);
				break;
			}
			break;

		case ClientPhase::STATUS:
			disconnect();
			break;
		}
	}

	void StateMachine::onClientSettingsChange(PacketClientSettings const& packet)
	{
		// TODO: validation
		auto oldVd = _playerState.clientSettings.viewDistance;
		_playerState.clientSettings.locale = std::string(packet.locale.data(), packet.locale.size());
		_playerState.clientSettings.viewDistance = std::max((Int8)2, std::min(packet.viewDistance, _globalState->serverSettings.maxViewDistance));
		_playerState.clientSettings.chatMode = (ChatMode)packet.chatMode;
		_playerState.clientSettings.chatColorsEnabled = packet.chatColors;
		_playerState.clientSettings.displayedSkinParts = packet.displayedSkinParts;
		_playerState.clientSettings.mainHand = (MainHand)packet.mainHand;

		// in initial state, view distance is set to 0
		if(oldVd != 0)
		{
			auto coord = coord_cast<ChunkCoord>(_playerState.position);
			auto vd = _playerState.clientSettings.viewDistance;

			auto lock = _globalState->playerTracker.lock();
			_globalState->playerTracker.updateViewDistance(coord, oldVd, vd, this);
			lock.unlock();

			if(oldVd > vd)
			{
				for(auto i = -oldVd; i <= oldVd; ++i)
				for(auto j = -oldVd; j <= oldVd; ++j)
				{
					if(i <= vd && j <= vd)
						continue;

					auto chunkCoord = coord + ChunkCoord{i, j};
					PacketUnloadChunk unload;
					unload.chunkX = chunkCoord.x;
					unload.chunkZ = chunkCoord.z;
					sendPacket(unload);
				}
			}
			else if(oldVd < vd)
			{
				for(auto i = -vd; i <= vd; ++i)
				for(auto j = -vd; j <= vd; ++j)
				{
					if(i <= oldVd && j <= oldVd)
						continue;

					sendChunk(coord + ChunkCoord{i, j});
				}
			}
		}
	}

	static
	std::unique_ptr<Chunk> generateChunk()
	{
		auto chunk = std::make_unique<Chunk>();
		chunk->sections[0] = std::make_unique<ChunkSection>();

		for(UInt8 x = 0; x != 16; ++x)
		for(UInt8 z = 0; z != 16; ++z)
			chunk->sections[0]->blocks[0][z][x] = BLOCKID_MINECRAFT_BEDROCK;

		for(UInt8 x = 0; x != 16; ++x)
		for(UInt8 z = 0; z != 16; ++z)
		for(UInt8 y = 1; y != 14; ++y)
			chunk->sections[0]->blocks[y][z][x] = BLOCKID_MINECRAFT_STONE;

		for(UInt8 x = 0; x != 16; ++x)
		for(UInt8 z = 0; z != 16; ++z)
			chunk->sections[0]->blocks[14][z][x] = BLOCKID_MINECRAFT_DIRT;

		for(UInt8 x = 0; x != 16; ++x)
		for(UInt8 z = 0; z != 16; ++z)
			chunk->sections[0]->blocks[15][z][x] = BLOCKID_MINECRAFT_GRASS_BLOCK;

		for(UInt8 x = 0; x != 16; ++x)
		for(UInt8 z = 0; z != 16; ++z)
			chunk->heightmap[z][x] = 16;

		return chunk;
	}

	Chunk* StateMachine::getOrCreateChunk(ChunkCoord coord)
	{
		{
			std::lock_guard guard(_globalState->chunkMutex);

			auto it = _globalState->chunks.find(coord);

			if(it != _globalState->chunks.end())
				return &*it->second;
		}

		auto chunk = generateChunk();
		auto result = &*chunk;

		std::lock_guard guard(_globalState->chunkMutex);
		_globalState->chunks[coord] = std::move(chunk);
		return result;
	}

	void StateMachine::sendChunk(ChunkCoord coord)
	{
		auto chunk = getOrCreateChunk(coord);
		std::lock_guard guard(chunk->mutex);

		UInt16 bitmask = 0;
		UInt sectionCount = 0;

		for(auto i = 0; i != 16; ++i)
			if(chunk->sections[i])
			{
				bitmask |= 1 << i;
				++sectionCount;
			}

		Int64 heightmap[36];
		bitpack16to9(&chunk->heightmap[0][0], sizeof chunk->heightmap / sizeof chunk->heightmap[0][0], (UInt8*)heightmap);

		Nbt heightmapNbt;
		heightmapNbt.type = NbtType::LONG_ARRAY;
		heightmapNbt.name = spanFromCString("MOTION_BLOCKING");
		heightmapNbt.value.ai64 = spanFromArray(heightmap);

		Buffer buffer;

		for(auto& section : chunk->sections)
		{
			if(!section)
				continue;

			serializeInt(buffer, (UInt16)(16 * 16 * 16)); // block count
			serializeInt(buffer, (UInt8)14); // bits per block
			// no palette

			Int64 data[16 * 16 * 16 * 14 / 64];
			bitpack16to14(&section->blocks[0][0][0], sizeof section->blocks / 2, (UInt8*)data);

			serializeVarInt(buffer, (Int32)(sizeof data / sizeof *data)); // length of data array in longs

			for(auto i : data)
				serializeInt(buffer, i);
		}

		for(int i = 0; i != 16; ++i)
			for(int j = 0; j != 16; ++j)
				serializeInt(buffer, chunk->biomes[i][j]);

		PacketChunkData chunkData;
		chunkData.x = coord.x;
		chunkData.z = coord.z;
		chunkData.fullChunk = true;
		chunkData.primaryBitmask = bitmask;
		chunkData.heightmaps.value.compound = Span(&heightmapNbt, 1);
		chunkData.data = Span((UInt8 const*)buffer.data(), buffer.size());
		chunkData.blockEntities = {};
		sendPacket(chunkData);
	}

	void StateMachine::onMove(EntityCoord oldPosition, bool rotate)
	{
		auto oldChunk = coord_cast<ChunkCoord>(oldPosition);
		auto newChunk = coord_cast<ChunkCoord>(_playerState.position);

		if(newChunk != oldChunk)
			onChunkTransition(oldChunk, newChunk, oldPosition, rotate);
		else
		{
			PacketEntityHeadLook packet;
			packet.entityId = _playerState.entityId;
			packet.headYaw = _playerState.yaw * 256 / 360;

			auto lock = _globalState->playerTracker.lock();
			broadcastLocallyUnsafe(createMovePacket(oldPosition, rotate), false);

			if(rotate)
				broadcastLocallyUnsafe(packet, false);
		}
	}

	Buffer StateMachine::createMovePacket(EntityCoord oldPosition, bool rotate)
	{
		auto diff = (_playerState.position - oldPosition) * 4096;
		Float64 min = -32768;
		Float64 max =  32767;

		if(!diff.withinOrdered(EntityCoord{min, min, min}, EntityCoord{max, max, max}))
		{
			PacketEntityTeleport packet;
			packet.entityId = _playerState.entityId;
			packet.x = _playerState.position.x;
			packet.y = _playerState.position.y;
			packet.z = _playerState.position.z;
			packet.yaw = _playerState.yaw * 256 / 360;
			packet.pitch = _playerState.pitch * 256 / 360;
			packet.onGround = false;
			return serializePacket(packet);
		}

		if(rotate)
		{
			PacketEntityMoveRotation packet;
			packet.entityId = _playerState.entityId;
			packet.dx = diff.x;
			packet.dy = diff.y;
			packet.dz = diff.z;
			packet.yaw = _playerState.yaw * 256 / 360;
			packet.pitch = _playerState.pitch * 256 / 360;
			packet.onGround = false;
			return serializePacket(packet);
		}

		PacketEntityMove packet;
		packet.entityId = _playerState.entityId;
		packet.dx = diff.x;
		packet.dy = diff.y;
		packet.dz = diff.z;
		packet.onGround = false;
		return serializePacket(packet);
	}

	Buffer StateMachine::createSpawnPacket(PlayerState const& state)
	{
		PacketSpawnPlayer spawn;
		spawn.entityId = state.entityId;
		spawn.uuid = state.uuid;
		spawn.x = state.position.x;
		spawn.y = state.position.y;
		spawn.z = state.position.z;
		spawn.yaw = state.yaw * 256 / 360;
		spawn.pitch = state.pitch * 256 / 360;

		UInt8 flags = 0;

		if(state.crouching)
			flags |= 0x02;

		if(state.sprinting)
			flags |= 0x08;

		spawn.metadata.push_back(EntityMetadata{0, EntityMetadataType::BYTE, flags});
		return serializePacket(spawn);
	}

	Buffer StateMachine::createDespawnPacket(PlayerState const& state)
	{
		PacketDestroyEntities destroy;
		destroy.entityIds.push_back({state.entityId});
		return serializePacket(destroy);
	}

	void StateMachine::sendMetadataUpdate()
	{
		PacketEntityMetadata packet;
		packet.entityId = _playerState.entityId;

		UInt8 flags = 0;

		if(_playerState.crouching)
			flags |= 0x02;

		if(_playerState.sprinting)
			flags |= 0x08;

		packet.metadata.push_back(EntityMetadata{0, EntityMetadataType::BYTE, flags});

		auto pose = _playerState.crouching ? EntityMetadataPose::CROUCHING : EntityMetadataPose::STANDING;
		packet.metadata.push_back(EntityMetadata{6, EntityMetadataType::POSE, pose});

		broadcastLocally(packet, false);
	}

	void StateMachine::onChunkTransition(ChunkCoord from, ChunkCoord to, EntityCoord oldPosition, bool rotate)
	{
		auto vd = _playerState.clientSettings.viewDistance;
		auto vdcoord = ChunkCoord{vd, vd};

		std::vector<ChunkCoord> addedChunks;
		std::vector<ChunkCoord> removedChunks;

		for(auto i = -vd; i <= vd; ++i)
		for(auto j = -vd; j <= vd; ++j)
		{
			if(auto coord = from + ChunkCoord{i, j}; !coord.withinOrdered(to - vdcoord, to + vdcoord))
				removedChunks.push_back(coord);

			if(auto coord = to + ChunkCoord{i, j}; !coord.withinOrdered(from - vdcoord, from + vdcoord))
				addedChunks.push_back(coord);
		}

		for(auto coord : removedChunks)
		{
			PacketUnloadChunk unload;
			unload.chunkX = coord.x;
			unload.chunkZ = coord.z;
			sendPacket(unload);
		}

		for(auto coord : addedChunks)
			sendChunk(coord);

		PacketUpdateViewPosition update;
		update.chunkX = to.x;
		update.chunkZ = to.z;
		sendPacket(update);

		auto lock = _globalState->playerTracker.lock();
		_globalState->playerTracker.move(from, to, this);
		_globalState->playerTracker.unsubscribe(removedChunks, this);
		_globalState->playerTracker.subscribe(addedChunks, this);

		auto fromSubs = _globalState->playerTracker.subscribers(from);
		auto toSubs = _globalState->playerTracker.subscribers(to);
		lock.unlock();

		auto movePacket = createMovePacket(oldPosition, rotate);
		auto spawnPacket = createSpawnPacket(_playerState);
		auto despawnPacket = createDespawnPacket(_playerState);

		PacketEntityHeadLook headLookPacket;
		headLookPacket.entityId = _playerState.entityId;
		headLookPacket.headYaw = _playerState.yaw * 256 / 360;

		for(auto& sub : fromSubs)
		{
			if(sub == this)
				continue;

			if(toSubs.find(sub) != toSubs.end())
			{
				sub->sendPacket(movePacket);

				if(rotate)
					sub->sendPacket(headLookPacket);
			}
			else
				sub->sendPacket(despawnPacket);
		}

		for(auto& sub : toSubs)
		{
			if(sub == this)
				continue;

			if(fromSubs.find(sub) == fromSubs.end())
				sub->sendPacket(spawnPacket);
		}
	}

	void StateMachine::onTick()
	{
		auto currentTime = _globalState->clock.now();

		if(currentTime - _lastPacketTime >= CLIENT_READ_TIMEOUT_NANOS)
		{
			disconnect("timeout");
			return;
		}

		if(_phase == ClientPhase::PLAY)
		{
			if(currentTime - _lastKeepAliveSentTime >= KEEP_ALIVE_INTERVAL_NANOS)
			{
				_lastKeepAliveSentTime = currentTime;

				PacketKeepAliveServer keepAlive;
				keepAlive.keepAliveId = currentTime;
				sendPacket(keepAlive);
			}
		}
	}

	StateMachine::~StateMachine()
	{
		if(_phase == ClientPhase::PLAY)
		{
			std::printf("player from %s left\n", _connection->endpoint().c_str());

			{
				auto coord = coord_cast<ChunkCoord>(_playerState.position);
				auto vd = _playerState.clientSettings.viewDistance;

				auto despawn = createDespawnPacket(_playerState);

				auto lock = _globalState->playerTracker.lock();
				_globalState->playerTracker.leave(coord, vd, this);
				broadcastLocallyUnsafe(despawn, false);
			}

			{
				PacketPlayerInfo::Entry entry;
				entry.uuid = _playerState.uuid;
				entry.update = PacketPlayerInfo::RemovePlayer();

				PacketPlayerInfo playerInfo;
				playerInfo.action = PacketPlayerInfo::REMOVE_PLAYER;
				playerInfo.entries.push_back(entry);

				std::lock_guard guard(_globalState->playersMutex);
				_globalState->players.erase(this);
				broadcastGloballyUnsafe(playerInfo, false);
			}
		}
	}
}
