#pragma once

#include <cassert>

#include <common/types.hpp>

#include <boost/container/small_vector.hpp>

namespace vitamine
{
	class Buffer
	{
		boost::container::small_vector<UInt8, 40> _data;

	public:
		void* data()
		{
			return _data.data();
		}

		UInt size()
		{
			return _data.size();
		}

		void discard(UInt size)
		{
			assert(size <= _data.size());
			_data.erase(_data.begin(), _data.begin() + size);
		}

		void write(void const* data, UInt size)
		{
			auto it = (UInt8 const*)data;
			_data.insert(_data.end(), it, it + size);
		}

		void prepend(void const* data, UInt size)
		{
			auto it = (UInt8 const*)data;
			_data.insert(_data.begin(), it, it + size);
		}

		void* prepareWrite(UInt size)
		{
			auto offset = _data.size();
			_data.resize(_data.size() + size);
			return _data.data() + offset;
		}
	};

	static_assert(sizeof(Buffer) == 64);
}
