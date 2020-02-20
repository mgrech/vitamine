#pragma once

#include <cstring>

#include <common/span.hpp>
#include <common/traits.hpp>
#include <common/types.hpp>

#include <boost/endian/conversion.hpp>
#include <boost/uuid/uuid.hpp>

namespace vitamine::proxyd
{
	constexpr UInt VARINT32_MAX_LENGTH = 5;

	enum struct DeserializeStatus
	{
		OK,
		ERROR_DATA_INCOMPLETE,
		ERROR_DATA_INVALID,
	};

	inline
	DeserializeStatus deserializeVarInt(UInt8 const** bufpp, UInt* sizep, Int32* out)
	{
		UInt32 result = 0;
		UInt count = 0;
		UInt8 tmp;

		do
		{
			if(count == VARINT32_MAX_LENGTH)
				return DeserializeStatus::ERROR_DATA_INVALID;

			if(count >= *sizep)
				return DeserializeStatus::ERROR_DATA_INCOMPLETE;

			tmp = (*bufpp)[count];
			result |= (tmp & 0x7fu) << (7 * count++);
		}
		while(tmp & 0x80u);

		*bufpp += count;
		*sizep -= count;
		*out = static_cast<Int32>(result);
		return DeserializeStatus::OK;
	}

	inline
	DeserializeStatus deserializeBytes(UInt8 const** bufpp, UInt* sizep, UInt blockSize, UInt8 const** out)
	{
		if(*sizep < blockSize)
			return DeserializeStatus::ERROR_DATA_INCOMPLETE;

		*out = *bufpp;
		*bufpp += blockSize;
		*sizep -= blockSize;
		return DeserializeStatus::OK;
	}

	template <typename T>
	DeserializeStatus deserializeInt(UInt8 const** bufpp, UInt* sizep, T* out)
	{
		UInt8 const* ptr;
		if(auto status = deserializeBytes(bufpp, sizep, sizeof(T), &ptr); status != DeserializeStatus::OK)
			return status;

		std::memcpy(out, ptr, sizeof(T));
		boost::endian::big_to_native_inplace(*out);
		return DeserializeStatus::OK;
	}

	template <typename T>
	DeserializeStatus deserializeFloat(UInt8 const** bufpp, UInt* sizep, T* out)
	{
		typename UIntForSize<sizeof(T)>::Type buf;

		if(auto status = deserializeInt(bufpp, sizep, &buf); status != DeserializeStatus::OK)
			return status;

		std::memcpy(out, &buf, sizeof buf);
		return DeserializeStatus::OK;
	}

	inline
	DeserializeStatus deserializeBool(UInt8 const** bufpp, UInt* sizep, bool* out)
	{
		UInt8 val;

		if(auto status = deserializeInt(bufpp, sizep, &val); status != DeserializeStatus::OK)
			return status;

		if(val > 1)
			return DeserializeStatus::ERROR_DATA_INVALID;

		*out = val != 0;
		return DeserializeStatus::OK;
	}

	inline
	DeserializeStatus deserializeString(UInt8 const** bufpp, UInt* sizep, Span<Char8 const>* out)
	{
		Int32 length;
		if(auto status = deserializeVarInt(bufpp, sizep, &length); status != DeserializeStatus::OK)
			return status;

		if(length < 0)
			return DeserializeStatus::ERROR_DATA_INVALID;

		UInt8 const* ptr;
		if(auto status = deserializeBytes(bufpp, sizep, length, &ptr); status != DeserializeStatus::OK)
			return status;

		*out = {(Char8 const*)ptr, (UInt)length};
		return DeserializeStatus::OK;
	}

	inline
	DeserializeStatus deserializeImplicitTailBytes(UInt8 const** bufpp, UInt* sizep, Span<UInt8 const>* out)
	{
		*out = Span<UInt8 const>(*bufpp, *sizep);
		*bufpp += *sizep;
		*sizep = 0;
		return DeserializeStatus::OK;
	}

	inline
	DeserializeStatus deserializeUuid(UInt8 const** bufpp, UInt* sizep, boost::uuids::uuid* out)
	{
		UInt64 lo, hi;

		if(auto status = deserializeInt(bufpp, sizep, &hi); status != DeserializeStatus::OK)
			return status;

		if(auto status = deserializeInt(bufpp, sizep, &lo); status != DeserializeStatus::OK)
			return status;

		boost::uuids::uuid uuid;
		std::memcpy(uuid.begin(),     &lo, 8);
		std::memcpy(uuid.begin() + 8, &hi, 8);
		*out = uuid;
		return DeserializeStatus::OK;
	}
}
