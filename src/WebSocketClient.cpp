#include "network-monitor/WebSocketClient.h"

using NetworkMonitor::WebSocketClient;

static void Log(const std::string& where, boost::system::error_code ec)
{
    std::cerr << "[" << std::setw(20) << where << "] "
              << (ec ? "Error: " : "OK")
              << (ec ? ec.message() : "")
              << std::endl;
}

WebSocketClient::WebSocketClient(
    const std::string& url,
    const std::string& endpoint,
    const std::string& port,
    net::io_context& ioc,
    net::ssl::context& ctx
) : m_url {url},
    m_endpoint {endpoint},
    m_port {port},
    resolver {net::make_strand(ioc)}, 
    ws {net::make_strand(ioc), ctx}
{
}

WebSocketClient::~WebSocketClient() = default;

void WebSocketClient::Connect(
    callbackPassingError onConnect,
    std::function<void (boost::system::error_code,
                        std::string&&)> onMessage,
    callbackPassingError onDisconnect
)
{
    m_onConnect = onConnect;
    m_onMessage = onMessage;
    m_onDisconnect = onDisconnect;

    closed = false;

    resolver.async_resolve(m_url, m_port, 
        [this](auto ec, auto resolverIt) {
            onResolve(ec, resolverIt);
        }
    );
}

void WebSocketClient::Send(
    const std::string& message,
    callbackPassingError onSend)
{
    net::const_buffer wbuffer {message.c_str(), message.size()};
    ws.async_write(wbuffer,
        [onSend](auto ec, auto size) {
            if(onSend)
                onSend(ec);
        }
    );
}

void WebSocketClient::Close(
    callbackPassingError onClose
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

void WebSocketClient::onResolve(boost::system::error_code ec, tcp::resolver::iterator resolverIt)
{
    if(ec){
        Log("OnResolve", ec);
        if (m_onConnect)
            m_onConnect(ec);
        
        return;
    }
    
    get_lowest_layer(ws).expires_after(std::chrono::seconds(5));
    get_lowest_layer(ws).async_connect(*resolverIt,
        [this](auto ec) {
            onConnect(ec);
        }
    );
}

void WebSocketClient::onConnect(boost::system::error_code ec)
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

void WebSocketClient::onTlsHandshake(boost::system::error_code ec)
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

void WebSocketClient::onHandshake(boost::system::error_code ec)
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

void WebSocketClient::ListenToIncomingMessage(
    const boost::system::error_code& ec
)
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

void WebSocketClient::onReceive(boost::system::error_code ec, std::size_t nBytes)
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