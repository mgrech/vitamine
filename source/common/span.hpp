#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

#include <common/types.hpp>

namespace vitamine::detail
{
	template <typename T, typename Derived>
	class SpanBase
	{
		T* _data;
		UInt _size;

	public:
		SpanBase() noexcept = default;
		constexpr SpanBase(decltype(nullptr)) noexcept : _data(nullptr), _size(0) {}
		constexpr SpanBase(SpanBase const&) noexcept = default;

		constexpr SpanBase(T* data, UInt size) noexcept
		: _data(data), _size(size)
		{}

		[[nodiscard]]
		constexpr T* data() const noexcept
		{
			return _data;
		}

		[[nodiscard]]
		constexpr UInt size() const noexcept
		{
			return _size;
		}

		template <typename U = T>
		[[nodiscard]]
		constexpr std::enable_if_t<!std::is_void_v<U>, U*> begin() const noexcept
		{
			return this->data();
		}

		template <typename U = T>
		[[nodiscard]]
		constexpr std::enable_if_t<!std::is_void_v<U>, U*> end() const noexcept
		{
			return this->data() + this->size();
		}

		template <typename U = T>
		[[nodiscard]]
		constexpr std::enable_if_t<!std::is_void_v<U>, U&> operator[](UInt index) const noexcept
		{
			assert(index < this->size());
			return this->data()[index];
		}

		template <typename U = T>
		[[nodiscard]]
		constexpr std::enable_if_t<!std::is_void_v<U>, Derived> subspan(UInt start, UInt end) const noexcept
		{
			assert(start < size());
			assert(end < size());
			return Derived(data() + start, end - start);
		}
	};
}

namespace vitamine
{
	template <typename T>
	class Span;

	template <>
	class Span<void const> : public detail::SpanBase<void const, Span<void const>>
	{
		using Base = detail::SpanBase<void const, Span<void const>>;

	public:
		Span() noexcept = default;
		using Base::Base;

		template <typename T>
		constexpr Span(std::vector<T>& v) noexcept
		: Base(v.data(), v.size())
		{}

		template <typename T>
		constexpr Span(std::vector<T> const& v) noexcept
		: Base(v.data(), v.size())
		{}

		template <typename T>
		constexpr Span(std::vector<T>&&) noexcept = delete;
		template <typename T>
		constexpr Span(std::vector<T> const&&) noexcept = delete;
	};

	template <>
	class Span<void> : public detail::SpanBase<void, Span<void>>
	{
		using Base = detail::SpanBase<void, Span<void>>;

	public:
		Span() noexcept = default;
		using Base::Base;

		template <typename T>
		constexpr Span(std::vector<T>& v) noexcept
		: Base(v.data(), v.size())
		{}

		[[nodiscard]]
		constexpr operator Span<void const>() const noexcept
		{
			return Span<void const>(data(), size());
		}
	};

	template <typename T>
	class Span<T const> : public detail::SpanBase<T const, Span<T const>>
	{
		using Base = detail::SpanBase<T const, Span<T const>>;

	public:
		Span() noexcept = default;
		using Base::Base;

		constexpr Span(std::vector<T>& v) noexcept
		: Base(v.data(), v.size())
		{}

		constexpr Span(std::vector<T> const& v) noexcept
		: Base(v.data(), v.size())
		{}

		constexpr Span(std::vector<T>&&) noexcept = delete;
		constexpr Span(std::vector<T> const&&) noexcept = delete;

		[[nodiscard]]
		constexpr operator Span<void const>() const noexcept
		{
			return Span<void const>(this->data(), this->size());
		}
	};

	template <typename T>
	class Span : public detail::SpanBase<T, Span<T>>
	{
		using Base = detail::SpanBase<T, Span<T>>;

	public:
		Span() noexcept = default;
		using Base::Base;

		constexpr Span(std::vector<T>& v) noexcept
		: Base(v.data(), v.size())
		{}

		[[nodiscard]]
		constexpr operator Span<void const>() const noexcept
		{
			return Span<void const>(this->data(), this->size());
		}

		[[nodiscard]]
		constexpr operator Span<void>() const noexcept
		{
			return Span<void>(this->data(), this->size());
		}

		[[nodiscard]]
		constexpr operator Span<T const>() const noexcept
		{
			return Span<T const>(this->data(), this->size());
		}
	};

	template <typename T>
	Span(T* data, UInt size) -> Span<T>;

	static_assert(sizeof(Span<void const>) == 2 * sizeof(void*));
	static_assert(sizeof(Span<void>)       == 2 * sizeof(void*));
	static_assert(sizeof(Span<char const>) == 2 * sizeof(void*));
	static_assert(sizeof(Span<char>)       == 2 * sizeof(void*));

	static_assert(std::is_trivial_v<Span<void const>>);
	static_assert(std::is_trivial_v<Span<void>>);
	static_assert(std::is_trivial_v<Span<char const>>);
	static_assert(std::is_trivial_v<Span<char>>);

	static_assert(std::is_convertible_v<Span<void>,       Span<void const>>);
	static_assert(std::is_convertible_v<Span<char const>, Span<void const>>);
	static_assert(std::is_convertible_v<Span<char>,       Span<void const>>);
	static_assert(std::is_convertible_v<Span<char>,       Span<void>>);
	static_assert(std::is_convertible_v<Span<char>,       Span<char const>>);
	static_assert(std::is_convertible_v<std::vector<char>&, Span<void const>>);
	static_assert(std::is_convertible_v<std::vector<char>&, Span<void>>);
	static_assert(std::is_convertible_v<std::vector<char>&, Span<char const>>);
	static_assert(std::is_convertible_v<std::vector<char>&, Span<char>>);
	static_assert(std::is_convertible_v<std::vector<char> const&, Span<void const>>);
	static_assert(std::is_convertible_v<std::vector<char> const&, Span<char const>>);

	template <typename T, UInt size>
	[[nodiscard]]
	constexpr Span<T> spanFromArray(T(& arr)[size]) noexcept
	{
		return Span(arr, size);
	}

	template <typename T, typename Traits, typename Alloc>
	Span<T const> spanFromStdString(std::basic_string<T, Traits, Alloc> const& str) noexcept
	{
		return Span(str.data(), str.size());
	}

	// prevent accidentally creating spans to temporaries
	template <typename T, typename Traits, typename Alloc>
	Span<T> spanFromStdString(std::basic_string<T, Traits, Alloc>&&) noexcept = delete;
	template <typename T, typename Traits, typename Alloc>
	Span<T const> spanFromStdString(std::basic_string<T, Traits, Alloc> const&&) noexcept = delete;

	template <typename T, UInt size>
	[[nodiscard]]
	constexpr Span<T const> spanFromCString(T const(& arr)[size]) noexcept
	{
		return Span(arr, size - 1);
	}

	template <typename T>
	[[nodiscard]]
	Span<T const> spanFromRuntimeCString(T const* str) noexcept
	{
		return Span(str, std::strlen(str));
	}

	template <typename T>
	bool operator==(Span<T> s1, Span<T> s2)
	{
		return s1.size() == s2.size() && std::equal(s1.begin(), s1.end(), s2.begin());
	}

	template <typename T>
	bool operator!=(Span<T> s1, Span<T> s2)
	{
		return !(s1 == s2);
	}

	template <typename T>
	std::basic_string<T, std::char_traits<T>, std::allocator<T>> toStdString(Span<T const> span)
	{
		return {span.data(), span.size()};
	}
}
