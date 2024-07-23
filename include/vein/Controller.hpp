#ifndef VEIN_CONTROLLER_HPP
#define VEIN_CONTROLLER_HPP

#include "vein/LibraryConfig.hpp"
#include "vein/HTML.hpp"

#include <boost/url/url_view.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <memory>


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
        for (auto const& param : url.params()) {
            if (!param.has_value) continue;
            if (auto it = doc_->name_tag.find(param.key); it != doc_->name_tag.end()) {
                it->second->attrs()["value"] = param.value;
            }
        }

        {
            auto const form_action = url.path();
            auto const form_it = doc_->form_action_tag.find(form_action);
            if (form_it == doc_->form_action_tag.end()) {
                throw std::invalid_argument{"no form found for this action"};
            }

            auto& callback = form_it->second->callback();
            if (!callback) {
                throw std::invalid_argument("callback was not set for this form");
            }

            callback(url.params());
        }

        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = html_->str();
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
