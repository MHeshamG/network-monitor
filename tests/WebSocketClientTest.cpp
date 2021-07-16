#include <network-monitor/WebSocketClient.h>

#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>

#include <string>
#include <openssl/ssl.h>
#include <filesystem>


using NetworkMonitor::WebSocketClient;

bool CheckResponse(const std::string& response)
{
    // We do not parse the whole message. We only check that it contains some
    // expected items.
    bool ok {true};
    ok &= response.find("ERROR") != std::string::npos;
    ok &= response.find("ValidationInvalidAuth") != std::string::npos;
    return ok;
}

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_CASE(class_WebSocketClient)
{
    // Connection targets
    const std::string url {"echo.websocket.org"};
    const std::string& endpoint{"/"};
    const std::string port {"443"};
    const std::string message {"Hello WebSocket"};

    // Always start with an I/O context object.
    net::io_context ioc {};

    //TLS Context
    net::ssl::context ctx(net::ssl::context::tlsv12_client);
    ctx.load_verify_file(TESTS_CACERT_PEM);

    // The class under test
    WebSocketClient client {url, endpoint, port, ioc, ctx};

    // We use these flags to check that the connection, send, receive functions
    // work as expected.
    bool connected {false};
    bool messageSent {false};
    bool messageReceived {false};
    bool disconnected {false};
    std::string echo {};

    // Our own callbacks
    auto onSend {[&messageSent](auto ec) {
        messageSent = !ec;
    }};
    auto onConnect {[&client, &connected, &onSend, &message](auto ec) {
        connected = !ec;
        if (!ec) {
            client.Send(message, onSend);
        }
    }};
    auto onClose {[&disconnected](auto ec) {
        disconnected = !ec;
    }};
    auto onReceive {[&client,
                      &onClose,
                      &messageReceived,
                      &echo](auto ec, auto received) {
        messageReceived = !ec;
        echo = std::move(received);
        client.Close(onClose);
    }};

    // We must call io_context::run for asynchronous callbacks to run.
    client.Connect(onConnect, onReceive);
    ioc.run();

    // When we get here, the io_context::run function has run out of work to do.
    BOOST_CHECK(connected);
    BOOST_CHECK(messageSent);
    BOOST_CHECK(messageReceived);
    BOOST_CHECK(disconnected);
    BOOST_CHECK_EQUAL(message, echo);
}

BOOST_AUTO_TEST_CASE(cacert_pem)
{
    BOOST_CHECK(std::filesystem::exists(TESTS_CACERT_PEM));
}

BOOST_AUTO_TEST_CASE(Valid_STOMP_Response)
{
    const std::string url {"wss://ltnm.learncppthroughprojects.com"};
    const std::string& endpoint{"/network-events"};
    const std::string port {"443"};

    // STOMP frame
    const std::string username {"fake_username"};
    const std::string password {"fake_password"};
    std::stringstream ss {};
    ss << "STOMP" << std::endl
       << "accept-version:1.2" << std::endl
       << "host:transportforlondon.com" << std::endl
       << "login:" << username << std::endl
       << "passcode:" << password << std::endl
       << std::endl // Headers need to be followed by a blank line.
       << '\0'; // The body (even if absent) must be followed by a NULL octet.
    const std::string message {ss.str()};

    // We use these flags to check that the connection, send, receive functions
    // work as expected.
    bool connected {false};
    bool messageSent {false};
    bool messageReceived {false};
    bool disconnected {false};
    std::string response {};

    // Always start with an I/O context object.
    net::io_context ioc {};

    //TLS Context
    net::ssl::context ctx(net::ssl::context::tlsv12_client);
    ctx.load_verify_file(TESTS_CACERT_PEM);

    // The class under test
    WebSocketClient client {url, endpoint, port, ioc, ctx};

    // Our own callbacks
    auto onSend {[&messageSent](auto ec) {
        messageSent = !ec;
    }};
    auto onConnect {[&client, &connected, &onSend, &message](auto ec) {
        connected = !ec;
        if (!ec) {
            client.Send(message, onSend);
        }
    }};
    auto onClose {[&disconnected](auto ec) {
        disconnected = !ec;
    }};
    auto onReceive {[&client,
                      &onClose,
                      &messageReceived,
                      &response](auto ec, auto received) {
        messageReceived = !ec;
        response = std::move(received);
        client.Close(onClose);
    }};

    // We must call io_context::run for asynchronous callbacks to run.
    client.Connect(onConnect, onReceive);
    ioc.run();

    // When we get here, the io_context::run function has run out of work to do.
    BOOST_CHECK(connected);
    BOOST_CHECK(messageSent);
    BOOST_CHECK(messageReceived);
    BOOST_CHECK(disconnected);
    BOOST_CHECK(CheckResponse(response));
}

BOOST_AUTO_TEST_SUITE_END();