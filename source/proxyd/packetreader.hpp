#pragma once

#include <functional>
#include <utility>

#include <common/buffer.hpp>
#include <common/span.hpp>
#include <common/types.hpp>
#include <proxyd/framing.hpp>

namespace vitamine::proxyd
{
	class PacketReader
	{
		Buffer _readBuffer;
		std::function<void(PacketFrame)> _onPacket;
		std::function<void()> _onError;

	public:
		PacketReader(std::function<void(PacketFrame)> onPacket, std::function<void()> onError)
		: _onPacket(std::move(onPacket)), _onError(std::move(onError))
		{}

		void onBytesReceived(Span<UInt8 const> data)
		{
			_readBuffer.write(data.data(), data.size());

			auto bufp = (UInt8 const*)_readBuffer.data();
			UInt size = data.size();

			PacketFrame frame;

			for(;;)
			{
				switch(deserializePacketFrame(&bufp, &size, &frame))
				{
				case DeserializeStatus::OK:
					_onPacket(frame);
					break;

				case DeserializeStatus::ERROR_DATA_INCOMPLETE:
					_readBuffer.discard(_readBuffer.size() - size);
					return;

				case DeserializeStatus::ERROR_DATA_INVALID:
					_onError();
					return;
				}
			}
		}
	};
}
