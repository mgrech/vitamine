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

			// TODO: find a better solution
			// creates shared_ptr ownership cycle, which needs to be cleared manually
			connection->userPointer(state);

			std::lock_guard lock(_mutex);
			_states.emplace(connection, std::move(state));
		}

		void onDataReceived(std::shared_ptr<IConnection> connection, Span<UInt8 const> data) final
		{
			auto state = std::static_pointer_cast<StateMachine>(connection->userPointer());
			state->onBytesReceived(data);
		}

		void onClientDisconnected(std::shared_ptr<IConnection> connection) final
		{
			// clears shared_ptr ownership cycle
			connection->userPointer(nullptr);

			std::lock_guard lock(_mutex);
			_states.erase(connection);
		}
	};
}
