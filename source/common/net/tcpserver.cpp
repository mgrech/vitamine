#include <common/net/tcpserver.hpp>

#include <atomic>
#include <utility>
#include <vector>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include <common/buffer.hpp>
#include <common/net/connection.hpp>
#include <common/net/connectionhandler.hpp>

namespace vitamine::detail
{
	class TcpServerConnection : public IConnection, public std::enable_shared_from_this<TcpServerConnection>
	{
		friend class TcpServerImpl;

		boost::asio::ip::tcp::socket _socket;
		boost::asio::io_service::strand _strand;
		ConnectionId _id;
		boost::asio::ip::tcp::endpoint _endpoint;
		std::atomic<bool> _disconnectMarker;

		UInt8 _readBuf[4096];
		std::vector<Buffer> _appendWriteQueue;
		std::vector<Buffer> _activeWriteQueue;
		std::vector<boost::asio::const_buffer> _activeWriteQueueBuffers;

		void* _userPtr;

		static
		void startWrite(std::shared_ptr<TcpServerConnection>&& connection)
		{
			if(connection->_appendWriteQueue.empty())
			{
				if(connection->_disconnectMarker)
					connection->_socket.close();

				return;
			}

			// preserve allocated capacity by using swap instead of move assignment
			connection->_activeWriteQueue.swap(connection->_appendWriteQueue);

			for(auto& buffer : connection->_activeWriteQueue)
				connection->_activeWriteQueueBuffers.emplace_back(buffer.data(), buffer.size());

			boost::asio::async_write(connection->_socket, connection->_activeWriteQueueBuffers, connection->_strand.wrap(
				[connection = std::move(connection)](boost::system::error_code ec, auto) mutable
				{
					connection->_activeWriteQueue.clear();
					connection->_activeWriteQueueBuffers.clear();

					if(!ec)
						connection->startWrite(std::move(connection));
				}));
		}

		static
		void sendImpl(std::shared_ptr<TcpServerConnection>&& connection, Buffer buffer)
		{
			connection->_appendWriteQueue.emplace_back(std::move(buffer));

			if(!connection->_activeWriteQueue.empty())
				return;

			startWrite(std::move(connection));
		}

		void disconnectImpl()
		{
			_disconnectMarker = true;

			if(_activeWriteQueue.empty())
			{
				boost::system::error_code ec;
				_socket.close(ec);
				// discard error: if the client closes the socket first, the socket is already closed
				// TODO: can this cause freshly connected clients to be disconnected under unlucky timing conditions?
			}
		}

	public:
		TcpServerConnection(boost::asio::io_service* service, ConnectionId id)
			: _socket(*service), _strand(*service), _id(id), _disconnectMarker(false)
		{}

		virtual ConnectionId id() const final
		{
			return _id;
		}

		virtual std::string endpoint() const final
		{
			return boost::lexical_cast<std::string>(_endpoint);
		}

		virtual void userPointer(void* ptr) final
		{
			_userPtr = ptr;
		}

		virtual void* userPointer() final
		{
			return _userPtr;
		}

		virtual void send(Buffer buffer) final
		{
			// discard packets after the connection has been marked for disconnect
			if(_disconnectMarker)
				return;

			_socket.get_io_service().post(_strand.wrap(
				[self = shared_from_this(), buffer = std::move(buffer)]() mutable
				{
					self->sendImpl(std::move(self), std::move(buffer));
				}));
		}

		virtual void disconnect() final
		{
			_socket.get_io_service().post(_strand.wrap(
				[self = shared_from_this()]
				{
					self->disconnectImpl();
				}));
		}
	};

	class TcpServerImpl
	{
		boost::asio::ip::tcp::acceptor _acceptor;
		ConnectionId _nextConnectionId = 1;
		IConnectionHandler* _handler;

		void startAccept()
		{
			auto connection = std::make_shared<TcpServerConnection>(&_acceptor.get_io_service(), _nextConnectionId++);

			_acceptor.async_accept(connection->_socket, connection->_endpoint,
			                       [this, connection = std::move(connection)](auto ec) mutable
			                       {
				                       if(ec)
				                       {
					                       if(ec == boost::asio::error::operation_aborted)
						                       return;

					                       throw boost::system::system_error(ec);
				                       }

				                       startAccept();
				                       connection->_socket.set_option(boost::asio::ip::tcp::no_delay(true), ec);

				                       // 'ec' is the result of setting TCP_NODELAY
				                       if(!ec)
				                       {
					                       _handler->onClientConnected(connection);
					                       startRead(std::move(connection));
				                       }
			                       });
		}

		void startRead(std::shared_ptr<TcpServerConnection>&& connection)
		{
			connection->_socket.async_read_some(boost::asio::buffer(connection->_readBuf), connection->_strand.wrap(
				[this, connection = std::move(connection)](boost::system::error_code ec, auto size) mutable
				{
					if(ec || connection->_disconnectMarker)
					{
						_handler->onClientDisconnected(std::move(connection));
						return;
					}

					_handler->onDataReceived(connection, {connection->_readBuf, size});
					startRead(std::move(connection));
				}));
		}

	public:
		TcpServerImpl(boost::asio::io_service* service, boost::asio::ip::tcp::endpoint endpoint, IConnectionHandler* handler)
		: _acceptor(*service, endpoint), _handler(handler)
		{}

		void asyncServe()
		{
			startAccept();
		}
	};
}

namespace vitamine
{
	TcpServer::TcpServer(boost::asio::io_service* service, boost::asio::ip::tcp::endpoint endpoint, IConnectionHandler* handler)
	: _impl(std::make_unique<detail::TcpServerImpl>(service, endpoint, handler))
	{}

	TcpServer::~TcpServer() = default;

	void TcpServer::asyncServe()
	{
		_impl->asyncServe();
	}
}
