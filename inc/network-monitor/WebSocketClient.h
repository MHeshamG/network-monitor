#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/system/error_code.hpp>

#include <openssl/ssl.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>


namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace websocket = boost::beast::websocket;
namespace beast = boost::beast;

using callbackPassingError = std::function<void (boost::system::error_code)>;

namespace NetworkMonitor {

/*! \brief Client to connect to a WebSocket server over plain TCP.
 *
 *  \tparam Resolver        The class to resolve the URL to an IP address. It
 *                          must support the same interface of
 *                          boost::asio::ip::tcp::resolver.
 *  \tparam WebSocketStream The WebSocket stream class. It must support the
 *                          same interface of boost::beast::websocket::stream.
 */
template <
    typename Resolver,
    typename WebSocketStream
>
class WebSocketClient {

public:
    /*! \brief Construct a WebSocket client.
     *
     *  \note This constructor does not initiate a connection.
     *
     *  \param url  The URL of the server.
     *  \param port The port on the server.
     *  \param ioc  The io_context object. The user takes care of calling
     *              ioc.run().
     *  \param ctx  The TLS context to setup a TLS socket stream
     */
    WebSocketClient(
        const std::string& url,
        const std::string& endpoint,
        const std::string& port,
        net::io_context& ioc,
        net::ssl::context& ctx
    ): m_url {url},
    m_endpoint {endpoint},
    m_port {port},
    resolver {net::make_strand(ioc)}, 
    ws {net::make_strand(ioc), ctx}
    {
        spdlog::info("WebSocketClient: New client for {}:{}{}",
                     m_url, m_port, m_endpoint);
    }

    /*! \brief Destructor.
     */
    ~WebSocketClient() = default;

    /*! \brief Connect to the server.
     *
     *  \param onConnect     Called when the connection fails or succeeds.
     *  \param onMessage     Called only when a message is successfully
     *                       received. The message is an rvalue reference;
     *                       ownership is passed to the receiver.
     *  \param onDisconnect  Called when the connection is closed by the server
     *                       or due to a connection error.
     */
    void Connect(
        callbackPassingError onConnect = nullptr,
        std::function<void (boost::system::error_code,
                            std::string&&)> onMessage = nullptr,
        callbackPassingError onDisconnect = nullptr
    )
    {
        std::cout<<m_url<<"................."<<m_port<<std::endl;
        m_onConnect = onConnect;
        m_onMessage = onMessage;
        m_onDisconnect = onDisconnect;

        closed = false;
        spdlog::info("WebSocketClient: Attempting to resolve {}:{}",
                     m_url, m_port);
        resolver.async_resolve(m_url, m_port, 
            [this](auto ec, auto resolverIt) {
                onResolve(ec, resolverIt);
            }
        );
    }

    /*! \brief Send a text message to the WebSocket server.
     *
     *  \param message The message to send. The caller must ensure that this
     *                 string stays in scope until the onSend handler is called.
     *  \param onSend  Called when a message is sent successfully or if it
     *                 failed to send.
     */
    void Send(
        const std::string& message,
        callbackPassingError onSend = nullptr
    )
    {
        net::const_buffer wbuffer {message.c_str(), message.size()};
        ws.async_write(wbuffer,
            [onSend](auto ec, auto size) {
                if(onSend)
                    onSend(ec);
            }
        );
    }

    /*! \brief Close the WebSocket connection.
     *
     *  \param onClose Called when the connection is closed, successfully or
     *                 not.
     */
    void Close(
        callbackPassingError onClose = nullptr
    )
    {
        closed = true;
        ws.async_close(
            websocket::close_code::none,
            [onClose](auto ec) {
                if (onClose) {
                    onClose(ec);
                }
            }
        );
    }

private:
    callbackPassingError m_onConnect{nullptr};
    std::function<void (boost::system::error_code, std::string&&)> m_onMessage{nullptr};
    callbackPassingError m_onDisconnect{nullptr};

    static void Log(const std::string& where, boost::system::error_code ec)
    {
        std::cerr << "[" << std::setw(20) << where << "] "
                << (ec ? "Error: " : "OK")
                << (ec ? ec.message() : "")
                << std::endl;
    }

    std::string m_url{};
    std::string m_endpoint{};
    std::string m_port{};

    bool closed {true};

    Resolver resolver;
    WebSocketStream ws;
    beast::flat_buffer rbuffer{};

    void onResolve(boost::system::error_code ec, tcp::resolver::iterator resolverIt)
    {
        if(ec){
            Log("OnResolvex", ec);
            if (m_onConnect)
                m_onConnect(ec);
            
            return;
        }
        spdlog::info("WebSocketClient: Server URL resolved: {}",
                     resolverIt->endpoint().address().to_string());
        get_lowest_layer(ws).expires_after(std::chrono::seconds(5));

        spdlog::info("WebSocketClient: Attempting connection to server");
        get_lowest_layer(ws).async_connect(*resolverIt,
            [this](auto ec) {
                onConnect(ec);
            }
        ); 
    }

    void onConnect(boost::system::error_code ec)
    {
        if(ec){
            Log("OnConnect", ec);
            if (m_onConnect)
                m_onConnect(ec);

            return;
        }

        // Now that the TCP socket is connected, we can reset the timeout to
        // whatever Boost.Beast recommends.
        // Note: The TCP layer is the lowest layer (WebSocket -> TLS -> TCP).
        get_lowest_layer(ws).expires_never();
        ws.set_option(websocket::stream_base::timeout::suggested(
            boost::beast::role_type::client
        ));

        // Some clients require that we set the host name before the TLS handshake
        // or the connection will fail. We use an OpenSSL function for that.
        SSL_set_tlsext_host_name(ws.next_layer().native_handle(), m_url.c_str());

        // Attempt a TLS handshake.
        // Note: The TLS layer is the next layer (WebSocket -> TLS -> TCP).
        ws.next_layer().async_handshake(net::ssl::stream_base::client,
                            [this](auto ec) {
                                onTlsHandshake(ec);
                            }
                        );
    }

    void onTlsHandshake(boost::system::error_code ec)
    {
        if(ec){
            Log("OnTLSHandShake",ec);
            if (m_onConnect)
                m_onConnect(ec);

            return;
        }
        ws.async_handshake(m_url, m_endpoint, 
                [this](auto ec) {
                    onHandshake(ec);
                }
            );
    }

    void onHandshake(boost::system::error_code ec)
    {
        if(ec){
            Log("OnHandshake", ec);
            if (m_onConnect)
                m_onConnect(ec);

            return;
        }

        // Tell the WebSocket object to exchange messages in text format.
        ws.text(true);

        ListenToIncomingMessage(ec);

        // Dispatch the user callback.
        // Note: This call is synchronous and will block the WebSocket strand.
        if (m_onConnect)
                m_onConnect(ec);
    }

    void ListenToIncomingMessage(const boost::system::error_code& ec)
    {
        // Stop processing messages if the connection has been aborted.
        if (ec == net::error::operation_aborted) {
            // We check the closed_ flag to avoid notifying the user twice.
            if (m_onDisconnect && ! closed) {
                m_onDisconnect(ec);
            }
            return;
        }

        // Read a message asynchronously. On a successful read, process the message
        // and recursively call this function again to process the next message.
        ws.async_read(rbuffer,
            [this](auto ec, auto nBytes) {
                onReceive(ec, nBytes);
                ListenToIncomingMessage(ec);
            }
        );
    }

    void onReceive(boost::system::error_code ec, std::size_t nBytes)
    {
        // We just ignore messages that failed to read.
        if (ec) {
            return;
        }

        // Parse the message and forward it to the user callback.
        // Note: This call is synchronous and will block the WebSocket strand.
        std::string message {boost::beast::buffers_to_string(rbuffer.data())};
        rbuffer.consume(nBytes);
        if (m_onMessage) {
            m_onMessage(ec, std::move(message));
        }
    }
};

using BoostWebSocketClient = WebSocketClient<
    boost::asio::ip::tcp::resolver,
    boost::beast::websocket::stream<
        boost::beast::ssl_stream<boost::beast::tcp_stream>
    >
>;

} // namespace NetworkMonitor
#endif // WEBSOCKET_CLIENT_H
