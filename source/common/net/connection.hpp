#pragma once

#include <string>

#include <common/buffer.hpp>
#include <common/types.hpp>

namespace vitamine
{
	using ConnectionId = UInt32;

	struct IConnection
	{
		[[nodiscard]]
		virtual ConnectionId id() const = 0;

		[[nodiscard]]
		virtual std::string endpoint() const = 0;

		virtual void userPointer(void* ptr) = 0;
		virtual void* userPointer() = 0;

		virtual void send(Buffer buffer) = 0;
		virtual void disconnect() = 0;

		virtual ~IConnection() = default;

	protected:
		IConnection() = default;
	};
}
