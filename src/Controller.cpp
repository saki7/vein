#include "pch.h"

#include "vein/Controller.hpp"

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
        [&](this auto&& self, html::Tag& tag) -> void {
            using html::TagType;

            if (tag.type() == TagType::title) {
                doc_->title_tag = &tag;
            }

            if (auto const it = tag.attrs().find("name"); it != tag.attrs().end()) {
                doc_->name_tag.emplace(std::get<std::string>(it->second), &tag);
            }
            if (auto const it = tag.attrs().find("id"); it != tag.attrs().end()) {
                doc_->id_tag.emplace(std::get<std::string>(it->second), &tag);
            }
            if (tag.type() == TagType::form) {
                std::string action;

                if (auto const it = tag.attrs().find("action"); it == tag.attrs().end()) {
                    action = "/";

                } else {
                    action = std::get<std::string>(it->second);
                    if (action.empty()) action = "/";
                }

                doc_->form_action_tag.emplace(std::move(action), &tag);
            }
            self(tag.contents());
        },
        [](this auto&& self, std::vector<html::TagContent>& contents) -> void {
            for (auto& content : contents) {
                yk::visit<void>(self, content);
            }
        },
        [](std::string const&) {},
    }(html_->contents());
}

void Controller::set_title(std::string title)
{
    reset_local_doc();

    if (!local_doc()->title_tag) {
        throw std::logic_error{"cannot set title because this html does not have title tag"};
    }

    local_doc()->title_tag->contents().clear();
    local_doc()->title_tag->append_string_content(std::move(title));
}

}
