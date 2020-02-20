#pragma once

#include <chrono>

#include <common/clock.hpp>

namespace vitamine
{
	class MonotonicClock : public Clock
	{
		using ClockImpl = std::chrono::steady_clock;

		ClockImpl::time_point _start = ClockImpl::now();

	public:
		virtual Int64 now() final
		{
			auto diff = ClockImpl::now() - _start;
			return std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count();
		}
	};
}
