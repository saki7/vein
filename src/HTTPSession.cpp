#include "pch.h"

#include "vein/HTTPSession.hpp"
#include "vein/File.hpp"
#include "vein/Router.hpp"


namespace vein {

void HTTPSession::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == http::error::end_of_stream) {
        return do_close();
    }
    if (ec == beast::error::timeout) {
        return; // prevent printing "The socket was closed due to a timeout"
    }
    if (ec) {
        return fail(ec, "read");
    }

#if VEIN_ENABLE_WEBSOCKET
    // See if it is a WebSocket Upgrade
    if (websocket::is_upgrade(parser_->get())) {
        // Create a websocket session, transferring ownership
        // of both the socket and the HTTP request.
        std::make_shared<WebSocketSession>(
            stream_.release_socket())->do_accept(parser_->release());
        return;
    }
#endif

    // Send the response
    queue_write(router_->handle_request(parser_->release()));

    // If we aren't at the queue limit, try to pipeline another request
    if (response_queue_.size() < queue_limit) {
        do_read();
    }
}

}
