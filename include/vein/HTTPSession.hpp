#ifndef VEIN_HTTP_SESSION_HPP
#define VEIN_HTTP_SESSION_HPP

#include "vein/Error.hpp"
#include "vein/WebSocketSession.hpp"

#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <queue>


namespace vein {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = beast::net;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;


class Router;

// Handles an HTTP server connection
class HTTPSession : public std::enable_shared_from_this<HTTPSession>
{
public:
    // Take ownership of the socket
    HTTPSession(tcp::socket&& socket, Router* router)
        : stream_(std::move(socket))
        , router_(router)
    {
        static_assert(queue_limit > 0, "queue limit must be positive");
    }

    // Start the session
    void
        run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(
            stream_.get_executor(),
            beast::bind_front_handler(
                &HTTPSession::do_read,
                this->shared_from_this()));
    }

private:
    void
        do_read()
    {
        // Construct a new parser for each message
        parser_.emplace();

        // Apply a reasonable limit to the allowed size
        // of the body in bytes to prevent abuse.
        parser_->body_limit(10000);

        // Set the timeout.
        stream_.expires_after(std::chrono::seconds(30));

        // Read a request using the parser-oriented interface
        http::async_read(
            stream_,
            buffer_,
            *parser_,
            beast::bind_front_handler(
                &HTTPSession::on_read,
                shared_from_this()));
    }

    void
        on_read(beast::error_code ec, std::size_t bytes_transferred);

    void
        queue_write(http::message_generator response)
    {
        // Allocate and store the work
        response_queue_.push(std::move(response));

        // If there was no previous work, start the write loop
        if (response_queue_.size() == 1)
            do_write();
    }

    // Called to start/continue the write-loop. Should not be called when
    // write_loop is already active.
    void
        do_write()
    {
        if (!response_queue_.empty()) {
            bool keep_alive = response_queue_.front().keep_alive();

            beast::async_write(
                stream_,
                std::move(response_queue_.front()),
                beast::bind_front_handler(
                    &HTTPSession::on_write,
                    shared_from_this(),
                    keep_alive));
        }
    }

    void
        on_write(
            bool keep_alive,
            beast::error_code ec,
            std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        if (!keep_alive) {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // Resume the read if it has been paused
        if (response_queue_.size() == queue_limit)
            do_read();

        response_queue_.pop();

        do_write();
    }

    void
        do_close()
    {
        // Send a TCP shutdown
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    Router* router_ = nullptr;

    static constexpr std::size_t queue_limit = 8; // max responses
    std::queue<http::message_generator> response_queue_;

    // The parser is stored in an optional container so we can
    // construct it from scratch it at the beginning of each new message.
    boost::optional<http::request_parser<http::string_body>> parser_;

};

}

#endif
