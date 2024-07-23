﻿#ifndef VEIN_ROUTER_HPP
#define VEIN_ROUTER_HPP

#include "vein/LibraryConfig.hpp"
#include "vein/Controller.hpp"
#include "vein/File.hpp"

#include <boost/url.hpp>
#include <boost/url/parse.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <string>
#include <unordered_map>


namespace vein {

namespace beast = boost::beast;
namespace http = beast::http;

using PathMatcher = std::string;


class Router
{
public:
    explicit Router(std::string const& doc_root);

    ~Router();

    // Append an HTTP rel-path to a local filesystem path.
    // The returned path is normalized for the platform.
    static std::string path_cat(std::string_view base, std::string_view path)
    {
        if (base.empty())
            return std::string(path);
        std::string result(base);

#ifdef BOOST_MSVC
        constexpr char path_separator = '\\';
        if (result.back() == path_separator) {
            result.resize(result.size() - 1);
        }
        result.append(path.data(), path.size());

        for (auto& c : result) {
            if (c == '/') {
                c = path_separator;
            }
        }
#else
        char constexpr path_separator = '/';
        if (result.back() == path_separator)
            result.resize(result.size() - 1);
        result.append(path.data(), path.size());
#endif
        return result;
    }

    std::string const& doc_root() const noexcept { return doc_root_; }

    void route(PathMatcher matcher, std::unique_ptr<Controller> controller);


    // Return a response for the given request.
    //
    // The concrete type of the response message (which depends on the
    // request), is type-erased in message_generator.
    template <class Body, class Allocator>
    http::message_generator handle_request(http::request<Body, http::basic_fields<Allocator>>&& req)
    {
        // Returns a bad request response
        auto const bad_request = [&req](beast::string_view why) {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = std::string(why);
            res.prepare_payload();
            return res;
        };

        // Returns a not found response
        auto const not_found = [&req](beast::string_view target) {
            http::response<http::string_body> res{http::status::not_found, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "The resource '" + std::string(target) + "' was not found.";
            res.prepare_payload();
            return res;
        };

        // Returns a server error response
        auto const server_error = [&req](beast::string_view what) {
            http::response<http::string_body> res{http::status::internal_server_error, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "An error occurred: '" + std::string(what) + "'";
            res.prepare_payload();
            return res;
        };

        // Make sure we can handle the method
        if (req.method() != http::verb::get &&
            req.method() != http::verb::head
        ) {
            return bad_request("Unknown HTTP-method");
        }

        // Request path must be absolute and not contain "..".
        if (req.target().empty() ||
            req.target()[0] != '/' ||
            req.target().find("..") != beast::string_view::npos
        ) {
            return bad_request("Illegal request-target");
        }


        // TODO: handle POST/HEAD request for controllers

        if (req.method() == http::verb::get) {
            auto const url = boost::urls::parse_origin_form(req.target());

            std::string const path = url->path();

            if (auto it = controllers_.find(path); it != controllers_.end()) {
                // app response
                auto const& controller = it->second;
                return controller->on_request(req, *url);
            }
        }

        // file response

        // Build the path to the requested file
        std::string path = path_cat(doc_root_, req.target());
        if (req.target().back() == '/') {
            path.append("index.html");
        }

        // Attempt to open the file
        beast::error_code ec;
        http::file_body::value_type body;
        body.open(path.c_str(), beast::file_mode::scan, ec);

        // Handle the case where the file doesn't exist
        if (ec == beast::errc::no_such_file_or_directory) {
            return not_found(req.target());
        }

        // Handle an unknown error
        if (ec) {
            return server_error(ec.message());
        }

        // Cache the size since we need it after the move
        auto const size = body.size();

        // Respond to HEAD request
        if (req.method() == http::verb::head) {
            http::response<http::empty_body> res{http::status::ok, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, mime_type(path));
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            return res;
        }

        // Respond to GET request
        http::response<http::file_body> res{
            std::piecewise_construct,
            std::make_tuple(std::move(body)),
            std::make_tuple(http::status::ok, req.version())};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }


private:
    std::string doc_root_ = ".";

    std::unordered_map<PathMatcher, std::unique_ptr<Controller>> controllers_;
};

}

#endif
