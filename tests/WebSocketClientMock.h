#ifndef NETWORK_MONITOR_TESTS_WEBSOCKET_CLIENT_MOCK_H
#define NETWORK_MONITOR_TESTS_WEBSOCKET_CLIENT_MOCK_H

#include <network-monitor/StompFrame.h>
#include <network-monitor/WebSocketClient.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <functional>
#include <iomanip>
#include <iostream>
#include <queue>
#include <string>
#include <utility>

namespace NetworkMonitor {

/*! \brief Mock the WebSocketClient class.
 *
 *  We do not mock all available methods — only the ones we are interested in
 *  for testing.
 */
class MockWebSocketClient {
public:
    // Use these static members in a test to set the error codes returned by
    // the mock.
    static boost::system::error_code connectEc;
    static boost::system::error_code sendEc;
    static boost::system::error_code closeEc;
    static bool triggerDisconnection;
    static std::queue<std::string> messageQueue;
    static std::function<void (const std::string&)> respondToSend;

    /*! \brief Mock constructor.
     */
    MockWebSocketClient(
        const std::string& url,
        const std::string& endpoint,
        const std::string& port,
        boost::asio::io_context& ioc,
        boost::asio::ssl::context& ctx
    );

    /*! \brief Mock destructor.
     */
    virtual ~MockWebSocketClient();

    /*! \brief Mock connection.
     */
    void Connect(
        std::function<void (boost::system::error_code)> onConnect = nullptr,
        std::function<void (boost::system::error_code,
                            std::string&&)> onMessage = nullptr,
        std::function<void (boost::system::error_code)> onDisconnect = nullptr
    );

    /*! \brief Send a mock message.
     */
    void Send(
        const std::string& message,
        std::function<void (boost::system::error_code)> onSend = nullptr
    );

    /*! \brief Mock close.
     */
    void Close(
        std::function<void (boost::system::error_code)> onClose = nullptr
    );



private:
    // This strand handles all the user callbacks.
    // We leave it uninitialized because it does not support a default
    // constructor.
    boost::asio::strand<boost::asio::io_context::executor_type> context_;

    bool connected_ {false};
    bool closed_ {false};

    void MockIncomingMessages(
        std::function<void (boost::system::error_code,
                            std::string&&)> onMessage = nullptr,
        std::function<void (boost::system::error_code)> onDisconnect = nullptr
    );
};

/*! \brief Mock the WebSocketClient class to connect to a STOMP server.
 *
 *  We do not mock all available methods — only the ones we are interested in
 *  for testing.
 */
class MockWebSocketClientForStomp: public MockWebSocketClient {
public:
    // Use these static members in a test to set the error codes returned by
    // the mock.
    // We also inherit all the lower-level controls for the underlying mock
    // WebSocket connection.
    static std::string endpoint; // For example: /passegners
    static std::string username;
    static std::string password;
    static std::vector<std::string> subscriptionMessages;

    /*! \brief Mock constructor.
     */
    MockWebSocketClientForStomp(
        const std::string& url,
        const std::string& endpoint,
        const std::string& port,
        boost::asio::io_context& ioc,
        boost::asio::ssl::context& ctx
    );

    static std::string GetMockSendFrame(
        const std::string& destination,
        const std::string& messageContent
    );

private:
    StompFrame MakeConnectedFrame();

    StompFrame MakeReceiptFrame(
        const std::string& id
    );

    StompFrame MakeErrorFrame(
        const std::string& msg
    );

    StompFrame MakeMessageFrame(
        const std::string& destination,
        const std::string& subscriptionId,
        const std::string& message
    );

    bool CheckConnection(
        const StompFrame& frame
    );

    std::pair<std::string, std::string> CheckSubscription(
        const StompFrame& frame
    );

    void OnMessage(
        const std::string& msg
    );
};

} // namespace NetworkMonitor

#endif // NETWORK_MONITOR_TESTS_BOOST_MOCK_H