#ifndef NETWORK_MONITOR_TESTS_BOOST_MOCK_H
#define NETWORK_MONITOR_TESTS_BOOST_MOCK_H

#include <network-monitor/WebSocketClient.h>

#include <boost/asio.hpp>
#include <boost/utility/string_view.hpp>

namespace NetworkMonitor
{

    class MockResolver
    {
    public:
        static boost::system::error_code resolveEc;

        template <typename ExecutionContext>
        explicit MockResolver(ExecutionContext &&context)
            : ctx(context)
        {
        }

        template <typename ResolverHandler>
        void async_resolve(
            boost::string_view host,
            boost::string_view service,
            ResolverHandler &&handler)
        {
            using resolver = boost::asio::ip::tcp::resolver;
            return boost::asio::async_initiate<ResolverHandler,
                                               void(boost::system::error_code, resolver::results_type)>(
                [](auto &&handler, auto resolver, auto host, auto service)
                {
                    if (MockResolver::resolveEc)
                    {
                        // Failing Branch
                        boost::asio::post(
                            resolver->ctx,
                            boost::beast::bind_handler(
                                std::move(handler),
                                MockResolver::resolveEc,
                                resolver::results_type{}));
                    }
                    else
                    {
                        // Successful Branch.
                        boost::asio::post(
                            resolver->ctx,
                            boost::beast::bind_handler(
                                std::move(handler),
                                MockResolver::resolveEc,
                                // Note: The create static method is in the public
                                //       resolver interface but it is not
                                //       documented.
                                resolver::results_type::create(
                                    boost::asio::ip::tcp::endpoint{
                                        boost::asio::ip::make_address(
                                            "127.0.0.1"),
                                        443},
                                    host.to_string(),
                                    service.to_string())));
                    }
                },
                handler,
                this,
                host,
                service);
        }

    private:
        boost::asio::strand<boost::asio::io_context::executor_type> ctx;
    };

    /*! \brief Mock the TCP socket stream from Boost.Beast.
    *
    *  We do not mock all available methods — only the ones we are interested in
    *  for testing.
    */
    class MockTcpStream : public boost::beast::tcp_stream
    {
    public:
        /*! \brief Inherit all constructors from the parent class.
        */
        using boost::beast::tcp_stream::tcp_stream;

        /*! \brief Use this static member in a test to set the error code returned
        *         by async_connect.
        */
        static boost::system::error_code connectEc;

        /*! \brief Mock for tcp_stream::async_connect
        */
        template <typename ConnectHandler>
        void async_connect(
            endpoint_type type,
            ConnectHandler &&handler)
        {
            return boost::asio::async_initiate<
                ConnectHandler,
                void(boost::system::error_code)>(
                [](auto &&handler, auto stream)
                {
                    // Call the user callback.
                    boost::asio::post(
                        stream->get_executor(),
                        boost::beast::bind_handler(
                            std::move(handler),
                            MockTcpStream::connectEc));
                },
                handler,
                this);
        }
    };

    // Out-of-line static member initialization
    inline boost::system::error_code MockTcpStream::connectEc{};

    // This overload is required by Boost.Beast when you define a custom stream.
    template <typename TeardownHandler>
    void async_teardown(
        boost::beast::role_type role,
        MockTcpStream &socket,
        TeardownHandler &&handler)
    {
        return;
    }

    template <typename TcpStream>
    class MockSslStream : public boost::beast::ssl_stream<TcpStream>
    {
    public:
        /*! \brief Inherit all constructors from the parent class.
        */
        using boost::beast::ssl_stream<TcpStream>::ssl_stream;

        /*! \brief Use this static member in a test to set the error code returned
        *         by async_handshake.
        */
        static boost::system::error_code handshakeEc;

        /*! \brief Mock for ssl_stream::async_connect
        */
        template <typename HandShakeHandler>
        void async_handshake(boost::asio::ssl::stream_base::handshake_type type, HandShakeHandler &&handler)
        {
            return boost::asio::async_initiate<
                HandShakeHandler,
                void(boost::system::error_code)>(
                [](auto &&handler, auto stream)
                {
                    // Call the user callback.
                    boost::asio::post(
                        stream->get_executor(),
                        boost::beast::bind_handler(
                            std::move(handler),
                            MockSslStream::handshakeEc));
                },
                handler,
                this);
        }
    };

    template <typename TcpStream>
    boost::system::error_code MockSslStream<TcpStream>::handshakeEc = {};

    // This overload is required by Boost.Beast when you define a custom stream.
    template <typename TeardownHandler>
    void async_teardown(
        boost::beast::role_type role,
        MockSslStream<MockTcpStream> &socket,
        TeardownHandler &&handler)
    {
        return;
    }

    template<typename TransportStream>
    class MockWebSocketStream: public boost::beast::websocket::stream<TransportStream>
    {
    public:
        /*! \brief Inherit all constructors from the parent class.
        */
        using boost::beast::websocket::stream<TransportStream>::stream;

        /*! \brief Use this static member in a test to set the error code returned
        *         by async_handshake.
        */
        static boost::system::error_code handshakeEc;

        /*! \brief Mock for ssl_stream::async_connect
        */
        template <typename HandShakeHandler>
        void async_handshake(boost::string_view host, boost::string_view target, HandShakeHandler &&handler)
        {
            return boost::asio::async_initiate<
                HandShakeHandler,
                void(boost::system::error_code)>(
                [](auto &&handler, auto stream, auto host, auto target)
                {
                    // Call the user callback.
                    boost::asio::post(
                        stream->get_executor(),
                        boost::beast::bind_handler(
                            std::move(handler),
                            MockWebSocketStream::handshakeEc));
                },
                handler,
                this,
                host.to_string(),
                target.to_string());
        }
    };

    template <typename TransportStream>
    boost::system::error_code MockWebSocketStream<TransportStream>::handshakeEc = {};

    inline boost::system::error_code MockResolver::resolveEc{};

    using MockTlsStream = MockSslStream<MockTcpStream>;

    using MockTlsWebSocketStream = MockWebSocketStream<MockTlsStream>;

    using TestWebSocketClient = WebSocketClient<
        MockResolver,
        MockTlsWebSocketStream
       >;

} //namespace NetworkMonitor

#endif