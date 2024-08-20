#include "pch.h"

#include "vein/Listener.hpp"
#include "vein/Router.hpp"


namespace vein {

Listener::Listener(
    net::io_context& ioc, tcp::endpoint endpoint, std::unique_ptr<Router> router
)
    : ioc_(ioc)
    , acceptor_(net::make_strand(ioc))
    , router_(std::move(router))
{
    beast::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        fail(ec, "open");
        return;
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        fail(ec, "set_option");
        return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
        fail(ec, "bind");
        return;
    }

    // Start listening for connections
    acceptor_.listen(
        net::socket_base::max_listen_connections, ec);
    if (ec) {
        fail(ec, "listen");
        return;
    }
}

Listener::~Listener() = default;

void Listener::on_accept(beast::error_code ec, tcp::socket socket)
{
    if (ec) {
        fail(ec, "accept");
    }
    else {
        // Create the http session and run it
        std::make_shared<HTTPSession>(
            std::move(socket),
            router_.get()
        )->run();
    }

    // Accept another connection
    do_accept();
}

}
