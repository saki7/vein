#include "pch.h"

#include "vein/Listener.hpp"
#include "vein/Router.hpp"


namespace vein {

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
