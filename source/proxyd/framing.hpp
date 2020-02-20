#pragma once

#include <common/buffer.hpp>
#include <common/span.hpp>
#include <common/types.hpp>
#include <proxyd/deserialize.hpp>
#include <proxyd/serialize.hpp>

namespace vitamine::proxyd::detail
{
	constexpr UInt INCOMING_PACKET_MAX_TOTAL_LENGTH = 1024;
	constexpr UInt INCOMING_PACKET_MAX_LENGTH_VALUE = INCOMING_PACKET_MAX_TOTAL_LENGTH - VARINT32_MAX_LENGTH;
}

namespace vitamine::proxyd
{
	struct PacketFrame
	{
		Int32 id;
		Span<UInt8 const> data;
	};

	inline
	DeserializeStatus deserializePacketFrame(UInt8 const** bufpp, UInt* sizep, PacketFrame* out)
	{
		auto bufp = *bufpp;
		auto size = *sizep;

		// packet format: [length, id, data]
		// length includes length of id
		Int32 length;
		if(auto status = deserializeVarInt(&bufp, &size, &length); status != DeserializeStatus::OK)
			return status;

		if(length < 0 || length > detail::INCOMING_PACKET_MAX_LENGTH_VALUE)
			return DeserializeStatus::ERROR_DATA_INVALID;

		UInt8 const* data;
		if(auto status = deserializeBytes(&bufp, &size, length, &data); status != DeserializeStatus::OK)
			return status;

		// at this point we have consumed the entire packet from the buffer
		// now we need to find out how many bytes are the id, the rest is data

		auto dataLength = static_cast<UInt>(length);

		Int32 id;
		if(auto status = deserializeVarInt(&data, &dataLength, &id); status != DeserializeStatus::OK)
			return status;

		*bufpp = bufp;
		*sizep = size;
		*out = {id, {data, dataLength}};
		return DeserializeStatus::OK;
	}

	template <typename Packet>
	Buffer serializePacket(Packet const& packet)
	{
		Buffer buffer;
		serializePacketPayload(buffer, packet);

		UInt8 idBuffer[detail::VARINT32_MAX_ENCODED_SIZE];
		auto idLength = detail::encodeVarInt32(idBuffer, Packet::ID);

		UInt8 headerBuffer[2 * detail::VARINT32_MAX_ENCODED_SIZE];
		auto lengthLength = detail::encodeVarInt32(headerBuffer, buffer.size() + idLength);

		// note: we intentionally use the entire buffer size instead of the actual length
		// this may copy uninitialized memory, but it's not actually read in that case
		// this is faster, because the size to memcpy is now a compile-time constant,
		// which means the compiler can just emit the copy instructions directly instead of calling memcpy
		// does it matter? probably not, but it's a cool trick
		std::memcpy(headerBuffer + lengthLength, idBuffer, sizeof idBuffer);

		buffer.prepend(headerBuffer, lengthLength + idLength);

		return buffer;
	}

	inline
	void debugPrint(PacketFrame frame)
	{
		std::printf("id=%d (hex: 0x%02x), length=%d (hex: 0x%x)\n", (int)frame.id, (int)frame.id, (int)frame.data.size(), (int)frame.data.size());

		for(auto x : frame.data)
			std::printf("%02x ", +x);

		std::printf("\n");
	}
}
