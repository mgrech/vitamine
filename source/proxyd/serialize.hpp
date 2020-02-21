#pragma once

#include <string>

#include <boost/endian/conversion.hpp>
#include <boost/uuid/uuid.hpp>

#include <common/bits.hpp>
#include <common/buffer.hpp>
#include <common/macros.hpp>
#include <common/span.hpp>
#include <common/traits.hpp>
#include <common/types.hpp>

namespace vitamine::proxyd
{
	namespace detail
	{
		constexpr UInt VARINT32_MAX_ENCODED_SIZE = ceildiv(32, 7);

		inline
		UInt wireSizeVarInt32(Int32 value)
		{
			switch(clz(value))
			{
			case 0:
			case 1:
			case 2:
			case 3:
				return 5;

			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
				return 4;

			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:
			case 17:
				return 3;

			case 18:
			case 19:
			case 20:
			case 21:
			case 22:
			case 23:
			case 24:
				return 2;

			case 25:
			case 26:
			case 27:
			case 28:
			case 29:
			case 30:
			case 31:
			case 32:
				return 1;

			default: UNREACHABLE
			}
		}

		inline
		UInt encodeVarInt32(UInt8* buf, Int32 value)
		{
			auto v = (UInt32)value;

			switch(clz(value))
			{
			case 0:
			case 1:
			case 2:
			case 3:
				buf[0] = 0x80u | v;
				buf[1] = 0x80u | v >> 7u;
				buf[2] = 0x80u | v >> 14u;
				buf[3] = 0x80u | v >> 21u;
				buf[4] =         v >> 28u;
				return 5;

			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
				buf[0] = 0x80u | v;
				buf[1] = 0x80u | v >> 7u;
				buf[2] = 0x80u | v >> 14u;
				buf[3] =         v >> 21u;
				return 4;

			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:
			case 17:
				buf[0] = 0x80u | v;
				buf[1] = 0x80u | v >> 7u;
				buf[2] =         v >> 14u;
				return 3;

			case 18:
			case 19:
			case 20:
			case 21:
			case 22:
			case 23:
			case 24:
				buf[0] = 0x80u | v;
				buf[1] =         v >> 7u;
				return 2;

			case 25:
			case 26:
			case 27:
			case 28:
			case 29:
			case 30:
			case 31:
			case 32:
				buf[0] = v;
				return 1;

			default: UNREACHABLE
			}
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
