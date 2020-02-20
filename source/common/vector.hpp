#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/functional/hash.hpp>

#include <common/traits.hpp>
#include <common/types.hpp>

namespace vitamine
{
	template <typename T, typename Tag = void>
	struct Vector2XZ
	{
		using ComponentType = T;

		T x, z;

		Vector2XZ() noexcept = default;
		constexpr Vector2XZ(T x, T z) noexcept : x(x), z(z) {}

		[[nodiscard]]
		bool withinOrdered(Vector2XZ from, Vector2XZ to) const
		{
			assert(from.x <= to.x);
			assert(from.z <= to.z);
			return x >= from.x && x <= to.x
			    && z >= from.z && z <= to.z;
		}

		template <typename U = T>
		[[nodiscard]]
		constexpr T lengthSquared() const
		{
			auto x = static_cast<U>(this->x);
			auto z = static_cast<U>(this->z);
			return x * x + z * z;
		}

		template <typename F = Float64>
		[[nodiscard]]
		F length() const
		{
			using std::sqrt;
			return sqrt(static_cast<F>(lengthSquared()));
		}
	};

	template <typename T, typename Tag = void>
	struct Vector3XYZ
	{
		using ComponentType = T;

		T x, y, z;

		Vector3XYZ() noexcept = default;
		constexpr Vector3XYZ(T x, T y, T z) noexcept : x(x), y(y), z(z) {}

		[[nodiscard]]
		bool withinOrdered(Vector3XYZ from, Vector3XYZ to)
		{
			assert(from.x <= to.x);
			assert(from.y <= to.y);
			assert(from.z <= to.z);
			return x >= from.x && x <= to.x
			    && y >= from.y && y <= to.y
			    && z >= from.z && z <= to.z;
		}

		template <typename U = T>
		[[nodiscard]]
		constexpr T lengthSquared() const
		{
			auto x = static_cast<U>(this->x);
			auto y = static_cast<U>(this->y);
			auto z = static_cast<U>(this->z);
			return x * x + y * y + z * z;
		}

		template <typename F = Float64>
		[[nodiscard]]
		F length() const
		{
			using std::sqrt;
			return sqrt(static_cast<F>(lengthSquared()));
		}
	};

	static_assert(std::is_trivial_v<Vector2XZ <Int>>);
	static_assert(std::is_trivial_v<Vector3XYZ<Int>>);

	static_assert(sizeof(Vector2XZ <Int>) == 2 * sizeof(Int));
	static_assert(sizeof(Vector3XYZ<Int>) == 3 * sizeof(Int));

	template <typename T, typename Tag>
	constexpr bool operator==(Vector2XZ<T, Tag> a, Vector2XZ<T, Tag> b)
	{
		return a.x == b.x && a.z == b.z;
	}

	template <typename T, typename Tag>
	constexpr bool operator==(Vector3XYZ<T, Tag> a, Vector3XYZ<T, Tag> b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}

	template <typename T, typename Tag>
	constexpr bool operator!=(Vector2XZ<T, Tag> a, Vector2XZ<T, Tag> b)
	{
		return !(a == b);
	}

	template <typename T, typename Tag>
	constexpr bool operator!=(Vector3XYZ<T, Tag> a, Vector3XYZ<T, Tag> b)
	{
		return !(a == b);
	}

	template <typename T, typename Tag>
	constexpr Vector2XZ<T, Tag>& operator+=(Vector2XZ<T, Tag>& a, Vector2XZ<T, Tag> b)
	{
		a.x += b.x;
		a.z += b.z;
		return a;
	}

	template <typename T, typename Tag>
	constexpr Vector3XYZ<T, Tag>& operator+=(Vector3XYZ<T, Tag>& a, Vector3XYZ<T, Tag> b)
	{
		a.x += b.x;
		a.y += b.y;
		a.z += b.z;
		return a;
	}

	template <typename T, typename Tag>
	constexpr Vector2XZ<T, Tag>& operator-=(Vector2XZ<T, Tag>& a, Vector2XZ<T, Tag> b)
	{
		a.x -= b.x;
		a.z -= b.z;
		return a;
	}

	template <typename T, typename Tag>
	constexpr Vector3XYZ<T, Tag>& operator-=(Vector3XYZ<T, Tag>& a, Vector3XYZ<T, Tag> b)
	{
		a.x -= b.x;
		a.y -= b.y;
		a.z -= b.z;
		return a;
	}

	template <typename T, typename Tag>
	constexpr Vector2XZ<T, Tag> operator+(Vector2XZ<T, Tag> a, Vector2XZ<T, Tag> b)
	{
		return {a.x + b.x, a.z + b.z};
	}

	template <typename T, typename Tag>
	constexpr Vector3XYZ<T, Tag> operator+(Vector3XYZ<T, Tag> a, Vector3XYZ<T, Tag> b)
	{
		return {a.x + b.x, a.y + b.y, a.z + b.z};
	}

	template <typename T, typename Tag>
	constexpr Vector2XZ<T, Tag> operator-(Vector2XZ<T, Tag> a, Vector2XZ<T, Tag> b)
	{
		return {a.x - b.x, a.z - b.z};
	}

	template <typename T, typename Tag>
	constexpr Vector3XYZ<T, Tag> operator-(Vector3XYZ<T, Tag> a, Vector3XYZ<T, Tag> b)
	{
		return {a.x - b.x, a.y - b.y, a.z - b.z};
	}

	template <typename T, typename Tag>
	constexpr Vector2XZ<T, Tag> operator*(Vector2XZ<T, Tag> v, typename Identity<T>::Type n)
	{
		return {v.x * n, v.z * n};
	}

	template <typename T, typename Tag>
	constexpr Vector3XYZ<T, Tag> operator*(Vector3XYZ<T, Tag> v, typename Identity<T>::Type n)
	{
		return {v.x * n, v.y * n, v.z * n};
	}

	template <typename T, typename Tag>
	constexpr Vector2XZ<T, Tag> operator*(typename Identity<T>::Type n, Vector2XZ<T, Tag> v)
	{
		return {n * v.x, n * v.z};
	}

	template <typename T, typename Tag>
	constexpr Vector3XYZ<T, Tag> operator*(typename Identity<T>::Type n, Vector3XYZ<T, Tag> v)
	{
		return {n * v.x, n * v.y, n * v.z};
	}


	template <typename T, typename Tag>
	constexpr Vector2XZ<T, Tag> operator/(Vector2XZ<T, Tag> v, typename Identity<T>::Type n)
	{
		return {v.x / n, v.z / n};
	}

	template <typename T, typename Tag>
	constexpr Vector3XYZ<T, Tag> operator/(Vector3XYZ<T, Tag> v, typename Identity<T>::Type n)
	{
		return {v.x / n, v.y / n, v.z / n};
	}

	template <typename T, typename Tag>
	constexpr Vector2XZ<T, Tag> operator/(typename Identity<T>::Type n, Vector2XZ<T, Tag> v)
	{
		return {n / v.x, n / v.z};
	}

	template <typename T, typename Tag>
	constexpr Vector3XYZ<T, Tag> operator/(typename Identity<T>::Type n, Vector3XYZ<T, Tag> v)
	{
		return {n / v.x, n / v.y, n / v.z};
	}

	template <typename T, typename Tag>
	std::string toString(Vector2XZ<T, Tag> v)
	{
		return '(' + std::to_string(v.x) + ',' + ' ' + std::to_string(v.z) + ')';
	}

	template <typename T, typename Tag>
	std::string toString(Vector3XYZ<T, Tag> v)
	{
		return '(' + std::to_string(v.x) + ',' + ' ' + std::to_string(v.y) + ',' + ' ' + std::to_string(v.z) + ')';
	}
}

namespace std
{
	template <typename T, typename Tag>
	struct hash<vitamine::Vector2XZ<T, Tag>>
	{
		std::size_t operator()(vitamine::Vector2XZ<T, Tag> v) const noexcept
		{
			return boost::hash_value(std::tie(v.x, v.z));
		}
	};

	template <typename T, typename Tag>
	struct hash<vitamine::Vector3XYZ<T, Tag>>
	{
		std::size_t operator()(vitamine::Vector3XYZ<T, Tag> v) const noexcept
		{
			return boost::hash_value(std::tie(v.x, v.y, v.z));
		}
	};

	template <typename T, typename Tag>
	struct less<vitamine::Vector2XZ<T, Tag>>
	{
		constexpr bool operator()(vitamine::Vector2XZ<T, Tag> a, vitamine::Vector2XZ<T, Tag> b) const noexcept
		{
			if(a.x < b.x)
				return true;

			if(a.x > b.x)
				return false;

			return a.z < b.z;
		}
	};

	template <typename T, typename Tag>
	struct less<vitamine::Vector3XYZ<T, Tag>>
	{
		constexpr bool operator()(vitamine::Vector3XYZ<T, Tag> a, vitamine::Vector3XYZ<T, Tag> b) const noexcept
		{
			if(a.x < b.x)
				return true;

			if(a.x > b.x)
				return false;

			if(a.y < b.y)
				return true;

			if(a.y > b.y)
				return false;

			return a.z < b.z;
		}
	};
}
