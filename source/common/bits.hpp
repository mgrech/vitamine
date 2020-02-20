#pragma once

#include <type_traits>

#include <common/traits.hpp>

namespace vitamine
{
	template <typename T>
	constexpr T pow2(T n) noexcept
	{
		return static_cast<T>(1) << n;
	}

	template <typename T>
	constexpr T nbitmask(T n) noexcept
	{
		return pow2(n) - static_cast<T>(1);
	}

	template <typename T>
	constexpr T ashr(T n, typename Identity<T>::Type shift)
	{
		using S = std::make_signed_t<T>;

		// signed shift-right is implementation-defined, but this way it's in one central place at least
		return static_cast<T>(static_cast<S>(n) >> static_cast<S>(shift));
	}

	template <typename TargetType, typename SourceType>
	constexpr TargetType zext(SourceType n)
	{
		using US = std::make_unsigned_t<SourceType>;
		using UT = std::make_unsigned_t<TargetType>;
		return static_cast<TargetType>(static_cast<UT>(static_cast<US>(n)));
	}

	template <typename T>
	constexpr T clz(T n)
	{
		return n == 0
		       ? 8 * sizeof n
		       : __builtin_clzll(zext<long long>(n)) - (64 - 8 * sizeof n);
	}

	template <typename T>
	constexpr T floorlog2(T n) noexcept
	{
		return static_cast<T>(8 * sizeof n - 1 - clz(n));
	}

	template <typename T>
	constexpr T ceildiv(T dividend, T divisor)
	{
		return (dividend + divisor - 1) / divisor;
	}
}
