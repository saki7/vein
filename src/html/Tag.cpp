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
    auto const type_str = to_string(type_);

    std::string res;

    if (attrs_.empty()) {
        res = std::format("<{}>", type_str);

    } else {
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

                        static std::regex const quotes_rgx{R"(")"};
                        res += std::regex_replace(v, quotes_rgx, "&quot;");

                        res += '"';
                    }
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
                [&](Tag const& tag) {
                    res += tag.str();
                },
            }, *content);
        }
    }

    if (!is_void_element()) {
        res += std::format("</{}>", type_str);
    }

    return res;
}

}
