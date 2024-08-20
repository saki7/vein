#include "pch.h"

#include "vein/html/Tag.hpp"

#include <yk/util/overloaded.hpp>
#include <yk/variant_view/boost.hpp>

#include <regex>
#include <algorithm>
#include <ranges>
#include <format>


namespace vein::html {

std::string Tag::str() const
{
    if (type_ == TagType::link && matches("rel", "canonical")) {
        auto const it = attrs_.find("href");
        if (it == attrs_.end()) return {};
        if (std::get<std::string>(it->second).empty()) return {};

    } else if (type_ == TagType::meta && matches("name", "description")) {
        auto const it = attrs_.find("content");
        if (it == attrs_.end()) return {};
        if (std::get<std::string>(it->second).empty()) return {};
    }

    auto const type_str = to_string(type_);

    std::string res;

    if (attrs_.empty()) {
        res = std::format("<{}>", type_str);

    } else {
        static std::regex const quotes_rgx{R"(")"};

        auto const attrs_str = attrs_ |
            std::views::transform([](auto const& kv) {
                auto const& [k, v] = kv;

                std::string res = k;

                std::visit(yk::overloaded{
                    [](std::monostate const&) {
                    },
                    [&](int const& v) {
                        res += '=';
                        res += std::to_string(v);
                    },
                    [&](std::string const& v) {
                        res += "=\"";
                        res += std::regex_replace(v, quotes_rgx, "&quot;");
                        res += '"';
                    },
                    [&](ClassList const& classes) {
                        res += "=\"";
                        res += classes
                            | std::views::transform([&](auto const& str) {
                                return std::regex_replace(str, quotes_rgx, "&quot;");
                            })
                            | std::views::join_with(std::string_view{" "})
                            | std::ranges::to<std::string>();
                        res += '"';
                    },
                }, v);
                return res;
            })
            | std::views::join_with(' ')
            | std::ranges::to<std::string>();

        res = std::format("<{} {}>", type_str, attrs_str);
    }

    if (!contents_.empty()) {
        if (is_void_element()) {
            throw std::invalid_argument{"void element cannot contain content"};
        }

        for (auto const& content : contents_) {
            yk::visit(yk::overloaded{
                [&](std::string const& str) {
                    res += str;
                },
                [&](TagPtr const& tag) {
                    res += tag->str();
                },
            }, content);
        }
    }

    if (!is_void_element()) {
        res += std::format("</{}>", type_str);
    }

    return res;
}

}
