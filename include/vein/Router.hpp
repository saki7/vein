#ifndef VEIN_ROUTER_HPP
#define VEIN_ROUTER_HPP

#include "vein/LibraryConfig.hpp"
#include "vein/Controller.hpp"
#include "vein/File.hpp"
#include "vein/Allocator.hpp"

#include <boost/url.hpp>
#include <boost/url/parse.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/http/vector_body.hpp>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/copy.hpp>

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>


namespace vein {

namespace beast = boost::beast;
namespace http = beast::http;

using PathMatcher = std::string;


class Router
{
public:
    explicit Router(std::filesystem::path const& public_root);

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

    void route(PathMatcher matcher, std::unique_ptr<Controller> controller);

    [[nodiscard]] static bool is_safe_path(std::filesystem::path root, std::filesystem::path child)
    {
        if (!exists(root)) return false;
        if (!exists(child)) return false;

        root = canonical(root);
        child = canonical(child);

        auto const it = std::search(child.begin(), child.end(), root.begin(), root.end());
        return it == child.begin();
    }

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
            //res.set(http::field::server, "vein");
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = std::string(why);
            res.prepare_payload();
            return res;
        };

        // Returns a not found response
        auto const not_found = [&req](beast::string_view target) {
            http::response<http::string_body> res{http::status::not_found, req.version()};
            //res.set(http::field::server, "vein");
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "The resource '" + std::string(target) + "' was not found.";
            res.prepare_payload();
            return res;
        };

        // Returns a server error response
        auto const server_error = [&req](beast::string_view what) {
            http::response<http::string_body> res{http::status::internal_server_error, req.version()};
            //res.set(http::field::server, "vein");
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
        if (req.target().empty() || req.target()[0] != '/') {
            return bad_request("Illegal request-target");
        }


        // TODO: handle POST/HEAD request for controllers

        auto const url = boost::urls::parse_origin_form(req.target());
        auto const url_path = url->path();
        if (url_path.contains("..")) {
            return not_found(req.target());
        }

        if (req.method() == http::verb::get) {
            // TODO: transparent hash

            if (auto it = controllers_.find(url_path.c_str()); it != controllers_.end()) {
                // app response
                auto const& controller = it->second;
                return controller->on_request(req, *url);
            }
        }

        // --------------------------------------------------
        // --------------------------------------------------
        // --------------------------------------------------
        // file response

        do {
            auto const slash_pos = url_path.find_last_of('/');
            if (slash_pos == boost::core::string_view::npos) break;
            if (slash_pos == url_path.size() - 1) break;

            if (url_path[slash_pos + 1] == '_') {
                // partial template
                return not_found(req.target());
            }
        } while (false);


        std::string_view rel_path = url_path;
        if (!rel_path.empty() && rel_path[0] == '/') {
            rel_path.remove_prefix(1);
        }

        std::filesystem::path const path = [this, &rel_path, &url_path] {
            auto path = public_root_ / std::string{rel_path};
            if (url_path.back() == '/') {
                path /= "index.html";
            }
            return path;
        }();

        if (!is_safe_path(public_root_, path)) {
            return not_found(req.target());
        }

        auto const mime = mime_type(path);

        if (mime.is_already_compressed) {
            // Attempt to open the file
            beast::error_code ec;
            http::file_body::value_type body;
            body.open(path.string().c_str(), beast::file_mode::scan, ec);

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

            if (req.method() == http::verb::head) {
                http::response<http::empty_body> res{http::status::ok, req.version()};
                //res.set(http::field::server, "vein");
                res.set(http::field::content_type, mime.type);
                res.content_length(size);
                res.keep_alive(req.keep_alive());
                return res;
            }

            http::response<http::file_body> res{
                std::piecewise_construct,
                std::make_tuple(std::move(body)),
                std::make_tuple(http::status::ok, req.version())
            };
            //res.set(http::field::server, "vein");
            res.set(http::field::content_type, mime.type);
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            return res;

        } else {
            http::response<http::vector_body<char, default_init_allocator<char>>> res{http::status::ok, req.version()};
            //res.set(http::field::server, "vein");
            res.set(http::field::content_type, mime.type);
            res.keep_alive(req.keep_alive());

            try {
                std::ifstream file{path.string(), std::ios::in | std::ios::binary};
                file.exceptions(std::ios::failbit | std::ios::badbit);

                boost::iostreams::filtering_istream is;

                is.push(boost::iostreams::zlib_compressor());
                is.push(file);
                // TODO: reserve?
                boost::iostreams::copy(is, std::back_inserter(res.body()));

                res.set(http::field::content_encoding, "deflate");

            } catch (std::ios::failure const& /*e*/) {
                //std::cerr << e.what(); << std::endl;
                return not_found(req.target());
            }

            res.prepare_payload();
            return res;
        }
    }


private:
    std::filesystem::path public_root_ = ".";

    std::unordered_map<PathMatcher, std::unique_ptr<Controller>> controllers_;
};

}

#endif
