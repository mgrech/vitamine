#pragma once

#include <memory>

#include <common/span.hpp>
#include <common/types.hpp>
#include <common/net/connection.hpp>

namespace vitamine
{
	struct IConnectionHandler
	{
		virtual void onClientConnected(std::shared_ptr<IConnection> connection) = 0;
		virtual void onDataReceived(std::shared_ptr<IConnection> connection, Span<UInt8 const> data) = 0;
		virtual void onClientDisconnected(std::shared_ptr<IConnection> connection) = 0;
		virtual ~IConnectionHandler() = default;

	protected:
		IConnectionHandler() = default;
	};
}
