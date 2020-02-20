#include <stdexcept>

#include <proxyd/nbt.hpp>
#include <proxyd/serialize.hpp>

namespace vitamine::proxyd
{
	// TODO: nbt strings are actually modified UTF-8:
	// https://docs.oracle.com/javase/8/docs/api/java/io/DataInput.html#modified-utf-8

	inline
	void serializeNbtValue(Buffer& buffer, NbtType type, NbtValue const& value)
	{
		switch(type)
		{
		case NbtType::BYTE:
			serializeInt(buffer, value.i8);
			break;

		case NbtType::SHORT:
			serializeInt(buffer, value.i16);
			break;

		case NbtType::INT:
			serializeInt(buffer, value.i32);
			break;

		case NbtType::LONG:
			serializeInt(buffer, value.i64);
			break;

		case NbtType::FLOAT:
			serializeFloat(buffer, value.f32);
			break;

		case NbtType::DOUBLE:
			serializeFloat(buffer, value.f64);
			break;

		case NbtType::STRING:
			serializeInt(buffer, (UInt16)value.str.size());
			serializeBytes(buffer, Span((UInt8 const*)value.str.data(), value.str.size()));
			break;

		case NbtType::BYTE_ARRAY:
			serializeInt(buffer, (Int32)value.ai8.size());
			serializeBytes(buffer, Span((UInt8 const*)value.ai8.data(), value.ai8.size()));
			break;

		case NbtType::INT_ARRAY:
			serializeInt(buffer, (Int32)value.ai32.size());

			for(auto i : value.ai32)
				serializeInt(buffer, i);

			break;

		case NbtType::LONG_ARRAY:
			serializeInt(buffer, (Int32)value.ai64.size());

			for(auto i : value.ai64)
				serializeInt(buffer, i);

			break;

		case NbtType::LIST:
			serializeInt(buffer, (UInt8)value.list.type);
			serializeInt(buffer, (Int32)value.list.values.size());

			for(auto& elem : value.list.values)
				serializeNbtValue(buffer, value.list.type, elem);

			break;

		case NbtType::COMPOUND:
			for(auto& elem : value.compound)
				serializeNbt(buffer, elem);

			// end marker
			serializeInt(buffer, (UInt8)0);
			break;
		}
	}

	void serializeNbt(Buffer& buffer, Nbt const& tag)
	{
		serializeInt(buffer, (UInt8)tag.type);
		serializeInt(buffer, (Int16)tag.name.size());
		serializeBytes(buffer, Span((UInt8 const*)tag.name.data(), tag.name.size()));
		serializeNbtValue(buffer, tag.type, tag.value);
	}

	DeserializeStatus deserializeNbt(UInt8 const** bufpp, UInt* sizep, Nbt* out)
	{
		// TODO: implement
		throw std::runtime_error("unimplemented");
	}
}
