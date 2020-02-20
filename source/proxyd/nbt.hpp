#pragma once

#include <common/buffer.hpp>
#include <common/span.hpp>
#include <common/types.hpp>
#include <proxyd/deserialize.hpp>

namespace vitamine::proxyd
{
	enum struct NbtType
	{
		BYTE       =  1,
		SHORT      =  2,
		INT        =  3,
		LONG       =  4,
		FLOAT      =  5,
		DOUBLE     =  6,
		BYTE_ARRAY =  7,
		STRING     =  8,
		LIST       =  9,
		COMPOUND   = 10,
		INT_ARRAY  = 11,
		LONG_ARRAY = 12,
	};

	struct Nbt;

	union NbtValue
	{
		Int8 i8;
		Int16 i16;
		Int32 i32;
		Int64 i64;
		Float32 f32;
		Float64 f64;
		Span<Char8 const> str;
		Span<Int8 const> ai8;
		Span<Int32 const> ai32;
		Span<Int64 const> ai64;
		struct { NbtType type; Span<NbtValue const> values; } list;
		Span<Nbt const> compound;

		NbtValue()
		: compound()
		{}
	};

	struct Nbt
	{
		NbtType type;
		Span<Char8 const> name;
		NbtValue value;

		Nbt()
		: type(NbtType::COMPOUND), name(), value()
		{}
	};

	void serializeNbt(Buffer& buffer, Nbt const& tag);
	DeserializeStatus deserializeNbt(UInt8 const** bufpp, UInt* sizep, Nbt* out);
}
