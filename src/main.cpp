#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast.hpp>

#include <iostream>
#include <thread>
#include <iomanip>

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
namespace net = boost::asio;
namespace beast = boost::beast;

void log(const std::string& where, boost::system::error_code ec)
{
    std::cerr << "[" << std::setw(20) << where << "] "
              << (ec ? "Error: " : "OK")
              << (ec ? ec.message() : "")
              << std::endl;
}

int main()
{
    const std::string url {"echo.websocket.org"};
    const std::string port {"80"};
    const std::string message {"Hello WebSocket"};

    net::io_context ioc{};
    tcp::socket socket{ioc};
    

    boost::system::error_code ec {};
    tcp::resolver resolver {ioc};
    auto resolverIt {resolver.resolve(url,port,ec)};
    if (ec) {
        log("resolver.resolve", ec);
        return -1;
    }

    socket.connect(*resolverIt, ec);
    if (ec) {
        log("socket.connect", ec);
        return -2;
    }

    websocket::stream<boost::beast::tcp_stream> ws(std::move(socket));
    ws.handshake(url,"/");
    if (ec) {
        log("ws.handshake", ec);
        return -3;
    }

    ws.text(true);

    net::const_buffer wbuffer {message.c_str(), message.size()};
    ws.write(wbuffer);
    if (ec) {
        log("ws.write", ec);
        return -4;
    }

    beast::flat_buffer rbuffer{};
    ws.read(rbuffer);
    if (ec) {
        log("ws.read", ec);
        return 51;
    }

    std::cout << "ECHO: "
              << beast::make_printable(rbuffer.data())
              << std::endl;

    log("returning", ec);

    ws.close(websocket::close_code::normal);

    return 0;
}