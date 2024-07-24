#ifndef VEIN_CONTROLLER_HPP
#define VEIN_CONTROLLER_HPP

#include "vein/html/Document.hpp"

#include <boost/url/url_view.hpp>

#include <boost/beast/http.hpp>

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
        http::response<http::string_body> res{status_code, req.version()};
        res.set(http::field::server, "vein");
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::move(response_body);
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
