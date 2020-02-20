#include <csignal>

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <common/net/tcpserver.hpp>
#include <proxyd/proxyserver.hpp>

int main()
{
	using namespace boost::asio;
	using namespace vitamine;
	using namespace vitamine::proxyd;

	auto endpoint = ip::tcp::endpoint(ip::tcp::v4(), 1337);

	io_service service;

	signal_set set(service);
	set.add(SIGINT);
	set.add(SIGTERM);
	set.async_wait([&](...){ service.stop(); });

	ProxyServer proxy(&service);
	TcpServer server(&service, endpoint, &proxy);
	server.asyncServe();

	service.run();
}
