#include "pch.h"

#include "vein/Server.hpp"
#include "vein/Listener.hpp"
#include "vein/Router.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>

#include <cstdlib>
#include <csignal>


namespace vein {

using tcp = boost::asio::ip::tcp;

int Server::wait(std::string const& host, unsigned port, std::unique_ptr<Router> router, unsigned thread_count)
{
    net::io_context ioc{static_cast<int>(thread_count)};

    // Create and launch a listening port
    auto l = std::make_shared<Listener>(
        ioc,
        tcp::endpoint{net::ip::make_address(host), static_cast<net::ip::port_type>(port)},
        std::move(router)
    );
    l->run();

    // Capture SIGINT and SIGTERM to perform a clean shutdown
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](beast::error_code const&, int) {
        // Stop the `io_context`. This will cause `run()`
        // to return immediately, eventually destroying the
        // `io_context` and all of the sockets in it.
        ioc.stop();
    });

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(thread_count - 1);

    for (auto i = static_cast<int>(thread_count) - 1; i > 0; --i) {
        v.emplace_back([&ioc] { ioc.run(); });
    }
    ioc.run();

    // (If we get here, it means we got a SIGINT or SIGTERM)

    // Block until all the threads exit
    for (auto& t : v) {
        t.join();
    }

    return EXIT_SUCCESS;
}

}
