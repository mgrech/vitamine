#pragma once

#include <memory>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace vitamine::detail
{
	class TcpServerImpl;
}

namespace vitamine
{
	struct IConnectionHandler;

	class TcpServer
	{
		std::unique_ptr<detail::TcpServerImpl> _impl;

	public:
		TcpServer(boost::asio::io_service* service, boost::asio::ip::tcp::endpoint endpoint, IConnectionHandler* handler);
		~TcpServer();

		void asyncServe();
	};
}
