#include "pch.h"

#include "vein/Controller.hpp"
#include "vein/Router.hpp"

#include <yk/util/overloaded.hpp>
#include <yk/variant/boost.hpp>
#include <yk/variant/std.hpp>


namespace vein {

Controller::Controller() = default;
Controller::~Controller() = default;

void Controller::reset_html(std::unique_ptr<html::Tag>& html_, std::unique_ptr<html::Document>& doc_, std::unique_ptr<html::Tag> html)
{
    html_ = std::move(html);
    doc_ = std::make_unique<html::Document>();

    yk::overloaded{
        [&](this auto&& self, html::TagPtr& tag) -> void {
            using html::TagType;

            if (tag->type() == TagType::head) {
                if (doc_->head_tag) {
                    throw std::logic_error{"<head>が複数あります"};
                }
                doc_->head_tag = tag.get();
            }
            if (tag->type() == TagType::link && tag->matches("rel", "canonical")) {
                if (doc_->link_rel_canonical_tag) {
                    throw std::logic_error{"<link rel=\"canonical\">が複数あります"};
                }
                doc_->link_rel_canonical_tag = tag.get();
            }
            if (tag->type() == TagType::meta && tag->matches("name", "description")) {
                if (doc_->description_tag) {
                    throw std::logic_error("<meta name=\"description\">が複数あります");
                }
                doc_->description_tag = tag.get();
            }

            if (tag->type() == TagType::title) {
                doc_->title_tag = tag.get();
            }

            if (tag->type() == TagType::body) {
                doc_->body_tag = tag.get();
            }

            if (auto const it = tag->attrs().find("name"); it != tag->attrs().end()) {
                doc_->name_tag.emplace(std::get<std::string>(it->second), tag.get());
            }
            if (auto const it = tag->attrs().find("id"); it != tag->attrs().end()) {
                doc_->id_tag.emplace(std::get<std::string>(it->second), tag.get());
            }
            if (tag->type() == TagType::form) {
                std::string action;

                if (auto const it = tag->attrs().find("action"); it == tag->attrs().end()) {
                    action = "/";

                } else {
                    action = std::get<std::string>(it->second);
                    if (action.empty()) action = "/";
                }

                doc_->form_action_tag.emplace(std::move(action), tag.get());
            }
            self(tag->contents());
        },
        [](this auto&& self, std::vector<html::TagContent>& contents) -> void {
            for (auto& content : contents) {
                yk::visit<void>(self, content);
            }
        },
        [](std::string const&) {},
    }(html_->contents());

    if (!doc_->head_tag) {
        throw std::invalid_argument{"<head>がありません"};
    }

    if (!doc_->description_tag) {
        auto tag = std::make_unique<html::Tag>(html::TagType::meta);
        tag->attrs().emplace("name", "description");
        tag->attrs().emplace("content", "");
        doc_->head_tag->contents().emplace_back(std::move(tag));
    }

    if (!doc_->link_rel_canonical_tag) {
        auto tag = std::make_unique<html::Tag>(html::TagType::link);
        tag->attrs().emplace("rel", "canonical");
        tag->attrs().emplace("href", "");
        doc_->head_tag->contents().emplace_back(std::move(tag));
    }
}

void Controller::set_title(std::string const& title)
{
    reset_local_doc();

    if (!local_doc()->title_tag) {
        throw std::logic_error{"cannot set title because this html does not have title tag"};
    }

    local_doc()->title_tag->contents().clear();
    local_doc()->title_tag->append_string_content(title);
}

void Controller::set_description(std::string const& description)
{
    reset_local_doc();
    local_doc()->description_tag->attrs()["content"] = description;
}

void Controller::set_link_rel_canonical(std::optional<boost::urls::url> const& link_rel_canonical)
{
    reset_local_doc();

    if (!link_rel_canonical) {
        local_doc()->link_rel_canonical_tag->attrs()["href"] = std::string{};
        return;
    }

    auto full_url = router_->canonical_url_origin();
    full_url.set_encoded_path(link_rel_canonical->encoded_path());
    full_url.set_encoded_query(link_rel_canonical->encoded_query());
    //full_url.set_encoded_fragment(link_rel_canonical->encoded_fragment());
    local_doc()->link_rel_canonical_tag->attrs()["href"] = std::string{full_url.c_str()};
}

}
