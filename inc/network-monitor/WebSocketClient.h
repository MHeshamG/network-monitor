#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast.hpp>

#include<string>
#include <functional>
#include <iostream>
#include <iomanip>


using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
namespace net = boost::asio;
namespace beast = boost::beast;

using callbackPassingError = std::function<void (boost::system::error_code)>;

namespace NetworkMonitor {

/*! \brief Client to connect to a WebSocket server over plain TCP.
 */
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
     */
    WebSocketClient(
        const std::string& url,
        const std::string& port,
        boost::asio::io_context& ioc
    );

    /*! \brief Destructor.
     */
    ~WebSocketClient();

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
    );

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
    );

    /*! \brief Close the WebSocket connection.
     *
     *  \param onClose Called when the connection is closed, successfully or
     *                 not.
     */
    void Close(
        callbackPassingError onClose = nullptr
    );
private:
    callbackPassingError m_onConnect{nullptr};
    std::function<void (boost::system::error_code, std::string&&)> m_onMessage{nullptr};
    callbackPassingError m_onDisconnect{nullptr};

    const std::string& m_url{};
    const std::string& m_port{};

    bool closed {true};

    tcp::resolver resolver;
    websocket::stream<boost::beast::tcp_stream> ws;
    beast::flat_buffer rbuffer{};

    void onResolve(boost::system::error_code, tcp::resolver::iterator);
    void onConnect(boost::system::error_code);
    void onHandShake(boost::system::error_code);
    void ListenToIncomingMessage(const boost::system::error_code& ec);
    void onReceive(boost::system::error_code, std::size_t);
};

} // namespace NetworkMonitor
#endif // WEBSOCKET_CLIENT_H
