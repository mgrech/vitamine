#pragma once

#include <common/types.hpp>

namespace vitamine
{
	template <typename T>
	struct Identity
	{
		using Type = T;
	};

	template <UInt n>
	struct UIntForSize;

	template <>
	struct UIntForSize<1> : Identity<UInt8>
	{};

	template <>
	struct UIntForSize<2> : Identity<UInt16>
	{};

	template <>
	struct UIntForSize<4> : Identity<UInt32>
	{};

	template <>
	struct UIntForSize<8> : Identity<UInt64>
	{};
}
