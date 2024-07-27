#ifndef VEIN_CONTROLLER_HPP
#define VEIN_CONTROLLER_HPP

#include "vein/html/Document.hpp"
#include "vein/Allocator.hpp"

#include <boost/url/url_view.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/http/vector_body.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/copy.hpp>

#include <memory>
#include <iostream>


namespace vein {

namespace beast = boost::beast;
namespace http = beast::http;

class Controller
{
public:
    Controller();
    ~Controller();

    void set_html(std::unique_ptr<html::Tag> html);

    template <class Body, class Allocator>
    http::message_generator on_request(http::request<Body, http::basic_fields<Allocator>> const& req, boost::urls::url_view url)
    {
        auto status_code = http::status::internal_server_error;
        std::string response_body;

        try {
            for (auto const& param : url.params()) {
                if (!param.has_value) continue;
                if (auto it = doc_->name_tag.find(param.key); it != doc_->name_tag.end()) {
                    it->second->attrs()["value"] = param.value;
                }
            }

            auto const form_action = url.path();
            auto const form_it = doc_->form_action_tag.find(form_action);
            if (form_it == doc_->form_action_tag.end()) {
                throw std::invalid_argument{"no form found for this action"};
            }

            auto& callback = form_it->second->callback();
            if (!callback) {
                throw std::invalid_argument("callback was not set for this form");
            }

            status_code = callback(url.params());
            response_body = html_->str();

        } catch (std::exception const& e) {
            std::cerr << "uncaught exception while dispatching controller: " << e.what() << std::endl;
            response_body = "Internal server error";

        } catch (...) {
            std::cerr << "uncaught and uncatchable exception while dispatching controller" << std::endl;
            response_body = "Internal server error";
        }

        http::response<http::vector_body<char, default_init_allocator<char>>> res{status_code, req.version()};
        res.set(http::field::server, "vein");
        res.set(http::field::content_type, "text/html; charset=utf-8");
        res.keep_alive(req.keep_alive());

        if (response_body.empty()) {
            res.prepare_payload();
            return res;
        }

        res.set(http::field::content_encoding, "deflate");
        {
            boost::iostreams::array_source src{response_body.data(), response_body.size()};
            boost::iostreams::filtering_istream is;
            is.push(boost::iostreams::zlib_compressor());
            is.push(src);

            res.body().reserve(response_body.size() / 8);
            boost::iostreams::copy(is, std::back_inserter(res.body()));
        }
        res.prepare_payload();
        return res;
    }

    html::Tag* tag_by_id(std::string const& id)
    {
        auto const it = doc_->id_tag.find(id);
        if (it == doc_->id_tag.end()) return nullptr;
        return it->second;
    }

private:
    std::unique_ptr<html::Tag> html_;
    std::unique_ptr<html::Document> doc_;
};

}

#endif
