#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <thread>
#include <iomanip>

using tcp = boost::asio::ip::tcp;

void log(boost::system::error_code ec)
{
    std::cerr << "[" << std::setw(14) << std::this_thread::get_id() << "] "
              << (ec ? "Error: " : "OK")
              << (ec ? ec.message() : "")
              << std::endl;
}

void onConnect(boost::system::error_code ec)
{
    log(ec);
}

int main()
{
    std::cerr << "[" << std::setw(14) << std::this_thread::get_id() << "] main"
              << std::endl;
    boost::asio::io_context ioc{};

    tcp::socket socket{boost::asio::make_strand(ioc)};

    size_t nThreads {4};

    // A 'falsey' error_code means "no error".
    boost::system::error_code ec {};
    tcp::resolver resolver {ioc};
    auto resolverIt {resolver.resolve("google.com","80",ec)};
    if (ec) {
        log(ec);
        return -1;
    }
    for (size_t idx {0}; idx < nThreads; ++idx) {
        socket.async_connect(*resolverIt, onConnect);
    }

    // We must call io_context::run for asynchronous callbacks to run.
    std::vector<std::thread> threads {};
    threads.reserve(nThreads);
    for (size_t idx {0}; idx < nThreads; ++idx) {
        threads.emplace_back([&ioc]() {
            ioc.run();
        });
    }
    for (size_t idx {0}; idx < nThreads; ++idx) {
        threads[idx].join();
    }
    return 0;
}