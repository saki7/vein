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
    virtual ~Controller();

    void set_html(std::unique_ptr<html::Tag> html)
    {
        reset_html(html_, doc_, std::move(html));
    }

    void clear_local_doc()
    {
        local_html().reset();
        local_doc().reset();
    }

    void set_title(std::string title);

    template<class F>
    void set_callback(std::string_view form_id, F&& f)
    {
        auto const it = doc_->id_tag.find(form_id);
        if (it == doc_->id_tag.end()) {
            throw std::invalid_argument{"form with specified ID was not found"};
        }
        it->second->callback() = std::forward<F>(f);
    }

    html::Tag* tag_by_id(std::string const& id)
    {
        reset_local_doc();

        auto const it = local_doc()->id_tag.find(id);
        if (it == local_doc()->id_tag.end()) return nullptr;
        return it->second;
    }

    template <class Body, class Allocator>
    http::message_generator on_request(http::request<Body, http::basic_fields<Allocator>> const& req, boost::urls::url_view url) const
    {
        auto status_code = http::status::internal_server_error;
        std::string response_body;

        try {
            reset_local_doc();

            for (auto const& param : url.params()) {
                if (!param.has_value) continue;
                if (auto it = local_doc()->name_tag.find(param.key); it != local_doc()->name_tag.end()) {
                    it->second->attrs()["value"] = param.value;
                }
            }

            auto const form_action = url.path();
            auto const form_it = local_doc()->form_action_tag.find(form_action);
            if (form_it == local_doc()->form_action_tag.end()) {
                throw std::invalid_argument{"no form found for this action"};
            }

            auto& callback = form_it->second->callback();
            if (!callback) {
                throw std::invalid_argument("callback was not set for this form");
            }

            status_code = callback(url.params());
            response_body = local_html()->str();

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

protected:
    Controller();

private:
    static void reset_html(std::unique_ptr<html::Tag>& html_, std::unique_ptr<html::Document>& doc_, std::unique_ptr<html::Tag> html);

    void reset_local_doc() const
    {
        if (!local_doc()) {
            reset_html(local_html(), local_doc(), std::make_unique<html::Tag>(*html_));
        }
    }

    virtual std::unique_ptr<html::Tag>& local_html() const = 0;
    virtual std::unique_ptr<html::Document>& local_doc() const = 0;

    std::unique_ptr<html::Tag> html_;
    std::unique_ptr<html::Document> doc_;
};


template<class Derived>
struct CustomController : Controller
{
    friend Controller;
    using Controller::Controller;

private:
    std::unique_ptr<html::Tag>& local_html() const override { return local_html_; }
    std::unique_ptr<html::Document>& local_doc() const override { return local_doc_; }

    static thread_local std::unique_ptr<html::Tag> local_html_;
    static thread_local std::unique_ptr<html::Document> local_doc_;
};

template<class Derived>
thread_local std::unique_ptr<html::Tag> CustomController<Derived>::local_html_;

template<class Derived>
thread_local std::unique_ptr<html::Document> CustomController<Derived>::local_doc_;

}

#endif
