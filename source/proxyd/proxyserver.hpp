#pragma once

#include <cassert>
#include <cstdio>
#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/system/system_error.hpp>

#include <common/span.hpp>
#include <common/types.hpp>
#include <common/net/connectionhandler.hpp>
#include <proxyd/globalstate.hpp>
#include <proxyd/statemachine.hpp>

namespace vitamine::proxyd
{
	constexpr UInt TICK_TIMER_PERIOD_MILLIS = 1000;

	class ProxyServer : public IConnectionHandler
	{
		GlobalState _globalState;

		mutable std::mutex _mutex;
		std::unordered_map<std::shared_ptr<IConnection>, std::shared_ptr<StateMachine>> _states;

		boost::asio::steady_timer _tickTimer;

		void startTickTimer()
		{
			_tickTimer.expires_from_now(std::chrono::milliseconds(TICK_TIMER_PERIOD_MILLIS));
			_tickTimer.async_wait([this](auto ec)
			{
				if(ec)
				{
					if(ec == boost::asio::error::operation_aborted)
						return;

					throw boost::system::system_error(ec);
				}

				startTickTimer();
				tickStateMachines();
			});
		}

		void tickStateMachines()
		{
			std::lock_guard lock(_mutex);

			for(auto&& [_, state] : _states)
				state->onTick();
		}

	public:
		explicit ProxyServer(boost::asio::io_service* service)
		: _tickTimer(*service)
		{
			startTickTimer();
		}

		void onClientConnected(std::shared_ptr<IConnection> connection) final
		{
			auto state = std::make_shared<StateMachine>(&_globalState, connection);

			// using shared_ptr instead would create an ownership cycle
			// storing a raw pointer is safe, because the state machine map holds a shared_ptr
			connection->userPointer(&*state);

			std::lock_guard lock(_mutex);
			_states.emplace(connection, std::move(state));
		}

		void onDataReceived(std::shared_ptr<IConnection> connection, Span<UInt8 const> data) final
		{
			auto state = static_cast<StateMachine*>(connection->userPointer());
			state->onBytesReceived(data);
		}

		void onClientDisconnected(std::shared_ptr<IConnection> connection) final
		{
			std::lock_guard lock(_mutex);
			_states.erase(connection);
		}
	};
}
