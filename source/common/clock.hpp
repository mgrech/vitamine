#pragma once

#include <common/types.hpp>

namespace vitamine
{
	struct Clock
	{
		[[nodiscard]]
		virtual Int64 now() = 0;

		virtual ~Clock() = default;
	};
}
