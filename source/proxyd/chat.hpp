#pragma once

#include <string>

#include <boost/format.hpp>

namespace vitamine::proxyd::chat
{
	inline
	std::string plainText(std::string const& text)
	{
		// TODO: escape characters
		return (boost::format(R"({"text":"%1%"})") % text).str();
	}
}
