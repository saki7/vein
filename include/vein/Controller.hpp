﻿#ifndef VEIN_CONTROLLER_HPP
#define VEIN_CONTROLLER_HPP

#include "vein/LibraryConfig.hpp"
#include "vein/html/Document.hpp"
#include "vein/HTTPField.hpp"

#include "yk/allocator/default_init_allocator.hpp"

#include <boost/url/url_view.hpp>
#include <boost/url/url.hpp>

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

class Router;

class Controller
{
public:
    virtual ~Controller();

    void set_router(Router* router) { router_ = router; }

    void set_html(std::unique_ptr<html::Tag> html)
    {
        reset_html(html_, doc_, std::move(html));
    }

    void clear_local_doc()
    {
        local_html().reset();
        local_doc().reset();
    }

    void set_title(std::string const& title);

    void set_description(std::string const& description);

    void set_link_rel_canonical(std::optional<boost::urls::url> const& link_rel_canonical);

    template<class F>
    void set_default_callback(F&& f)
    {
        static_assert(std::is_invocable_r_v<http::status, F, boost::urls::url_view const&, HTTPFields&>);
        doc_->default_callback_ = std::forward<F>(f);
    }

    template<class F>
    void set_form_callback(std::string_view form_id, F&& f)
    {
        static_assert(std::is_invocable_r_v<http::status, F, boost::urls::url_view const&, HTTPFields&>);

        auto const it = doc_->id_tag.find(form_id);
        if (it == doc_->id_tag.end()) {
            throw std::invalid_argument{"form with specified ID was not found"};
        }
        it->second->callback() = std::forward<F>(f);
    }

    auto* tag_by_id(this auto&& self, std::string_view id)
    {
        self.reset_local_doc();
        return self.local_doc()->tag_by_id(id);
    }

    auto* body_tag(this auto&& self)
    {
        self.reset_local_doc();
        return self.local_doc()->body_tag;
    }

    template <class Body, class Allocator>
    http::message_generator on_request(http::request<Body, http::basic_fields<Allocator>> const& req, boost::urls::url_view url) const
    {
        auto status_code = http::status::ok;
        std::string response_body;

        static thread_local HTTPFields http_fields;
        http_fields.clear();

        try {
            reset_local_doc();

            //for (auto const& param : url.params()) {
            //    if (!param.has_value) continue;
            //    if (auto it = local_doc()->name_tag.find(param.key); it != local_doc()->name_tag.end()) {
            //        it->second->attrs()["value"] = param.value;
            //    }
            //}

            auto const form_action = url.path();

            do {
                if (auto const form_it = local_doc()->form_action_tag.find(form_action);
                    form_it == local_doc()->form_action_tag.end()
                ) {
                    if (!doc_->default_callback_) {
                        std::cerr << "warning: the url does not match any form actions, and the default callback on the controller was unset" << std::endl;
                        break;
                    }
                    status_code = doc_->default_callback_(url, http_fields);

                } else {
                    auto& callback = form_it->second->callback();
                    if (!callback) {
                        throw std::invalid_argument("callback was not set for this form");
                    }

                    status_code = callback(url, http_fields);
                }
            } while (false);

            if (auto const code = std::to_underlying(status_code); 300 <= code && code <= 399) {
                // ...

            } else {
                response_body = "<!DOCTYPE html>\n" + local_html()->str();
            }

        } catch (std::exception const& e) {
            std::cerr << "uncaught exception while dispatching controller: " << e.what() << std::endl;
            status_code = http::status::internal_server_error;
            response_body = "Internal server error";

        } catch (...) {
            std::cerr << "uncaught and uncatchable exception while dispatching controller" << std::endl;
            status_code = http::status::internal_server_error;
            response_body = "Internal server error";
        }

        http::response<http::vector_body<char, yk::default_init_allocator<char>>> res{status_code, req.version()};

        for (auto const& [field, value] : http_fields) {
            res.set(field, value);
        }
        if (!http_fields.contains(http::field::content_type)) {
            res.set(http::field::content_type, "text/html; charset=utf-8");
        }

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

protected:
    Controller();

    [[nodiscard]] Router* router() const noexcept { return router_; }

    [[nodiscard]] html::Document const* doc() const noexcept { return doc_.get(); }
    [[nodiscard]] html::Document* doc() noexcept { return doc_.get(); }

private:
    static void reset_html(std::unique_ptr<html::Tag>& html_, std::unique_ptr<html::Document>& doc_, std::unique_ptr<html::Tag> html);

    void reset_local_doc() const
    {
        if (!local_doc()) {
            reset_html(local_html(), local_doc(), std::make_unique<html::Tag>(*html_));
            local_doc()->default_callback_ = doc_->default_callback_;
        }
    }

    virtual std::unique_ptr<html::Tag>& local_html() const = 0;
    virtual std::unique_ptr<html::Document>& local_doc() const = 0;

    mutable Router* router_ = nullptr;

    std::unique_ptr<html::Tag> html_;
    std::unique_ptr<html::Document> doc_;
};


template<class Derived>
struct CustomController : Controller
{
    friend Controller;

protected:
    using Controller::Controller;

private:
    std::unique_ptr<html::Tag>& local_html() const override
    {
        static thread_local std::unique_ptr<html::Tag> local_html_;
        return local_html_;
    }
    std::unique_ptr<html::Document>& local_doc() const override
    {
        static thread_local std::unique_ptr<html::Document> local_doc_;
        return local_doc_;
    }
};

}

#endif
