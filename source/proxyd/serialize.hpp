#pragma once

#include <string>

#include <boost/endian/conversion.hpp>
#include <boost/uuid/uuid.hpp>

#include <common/bits.hpp>
#include <common/buffer.hpp>
#include <common/span.hpp>
#include <common/traits.hpp>
#include <common/types.hpp>

namespace vitamine::proxyd
{
	namespace detail
	{
		constexpr UInt VARINT32_MAX_ENCODED_SIZE = ceildiv(32, 7);

		inline
		UInt encodeVarInt32(UInt8* buf, Int32 value)
		{
			auto val = static_cast<UInt32>(value);
			UInt size = 1;

			while(val >> 7u)
			{
				*buf++ = 0x80u | (val & 0x7fu);
				val >>= 7u;
				++size;
			}

			*buf = val;
			return size;
		}
	}

	inline
	void serializeVarInt(Buffer& buffer, Int32 value)
	{
		UInt8 buf[detail::VARINT32_MAX_ENCODED_SIZE];
		auto length = detail::encodeVarInt32(buf, value);
		buffer.write(buf, length);
	}

	template <typename T>
	void serializeInt(Buffer& buffer, T value)
	{
		boost::endian::native_to_big_inplace(value);
		buffer.write(&value, sizeof value);
	}

	inline
	void serializeBool(Buffer& buffer, bool value)
	{
		serializeInt(buffer, (UInt8)value);
	}

	template <typename T>
	void serializeFloat(Buffer& buffer, T value)
	{
		typename UIntForSize<sizeof(T)>::Type intval;
		std::memcpy(&intval, &value, sizeof value);
		boost::endian::native_to_big_inplace(intval);
		buffer.write(&intval, sizeof intval);
	}

	inline
	void serializeString(Buffer& buffer, Span<Char8 const> str)
	{
		serializeVarInt(buffer, str.size());
		buffer.write(str.data(), str.size());
	}

	inline
	void serializeString(Buffer& buffer, std::string const& str)
	{
		serializeString(buffer, Span<Char8 const>(str.data(), str.size()));
	}

	inline
	void serializeBytes(Buffer& buffer, Span<UInt8 const> data)
	{
		buffer.write(data.data(), data.size());
	}

	inline
	void serializeUuid(Buffer& buffer, boost::uuids::uuid uuid)
	{
		UInt64 lo, hi;
		std::memcpy(&lo, uuid.begin(),     8);
		std::memcpy(&hi, uuid.begin() + 8, 8);
		serializeInt(buffer, hi);
		serializeInt(buffer, lo);
	}
}
