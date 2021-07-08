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
    const std::string& port,
    net::io_context& ioc
) : m_url {url}, 
    m_port {port}, 
    resolver {net::make_strand(ioc)}, 
    ws {net::make_strand(ioc)}
{
    ws.text(true);
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
    
    ws.next_layer().expires_after(std::chrono::seconds(5));
    ws.next_layer().async_connect(*resolverIt,
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
    ws.next_layer().expires_never();
    ws.set_option(websocket::stream_base::timeout::suggested(
        boost::beast::role_type::client
    ));
    ws.async_handshake(m_url, "/", 
        [this](auto ec) {
            onHandShake(ec);
        }
    );
}

void WebSocketClient::onHandShake(boost::system::error_code ec)
{
    if(ec){
        Log("OnHandshake", ec);
        if (m_onConnect)
            m_onConnect(ec);

        return;
    }

    ws.text(true);

    ListenToIncomingMessage(ec);

    if (m_onConnect)
            m_onConnect(ec);
}

void WebSocketClient::ListenToIncomingMessage(
    const boost::system::error_code& ec
)
{
    if (ec == net::error::operation_aborted) {
        if (m_onDisconnect && ! closed) {
            m_onDisconnect(ec);
        }
        return;
    }

    
    ws.async_read(rbuffer,
        [this](auto ec, auto nBytes) {
            onReceive(ec, nBytes);
            ListenToIncomingMessage(ec);
        }
    );
}

void WebSocketClient::onReceive(boost::system::error_code ec, std::size_t nBytes)
{
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