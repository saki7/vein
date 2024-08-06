#ifndef VEIN_HTML_TAG_HPP
#define VEIN_HTML_TAG_HPP

#include "vein/Hash.hpp"

#include <yk/util/overloaded.hpp>
#include <yk/variant/boost.hpp>

#include <boost/variant/variant.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/url/params_view.hpp>

#include <unordered_map>
#include <vector>
#include <memory>
#include <ranges>
#include <functional>
#include <variant>
#include <string>
#include <array>
#include <type_traits>


namespace vein {
namespace beast = boost::beast;
namespace http = beast::http;
} // vein


namespace vein::html {

enum class TagType : int
{
    div,
    p,
    ul,
    ol,
    li,
    form,
    input,
    button,
    main,
    body,
    link,
    style,
    script,
    meta,
    title,
    head,
    html,
    _last_ = html,
};

inline constexpr auto tag_type_names_v = std::array{
    "div",
    "p",
    "ul",
    "ol",
    "li",
    "form",
    "input",
    "button",
    "main",
    "body",
    "link",
    "style",
    "script",
    "meta",
    "title",
    "head",
    "html",
};

constexpr bool is_void_element(TagType type) noexcept
{
    using enum TagType;

    switch (type) {
    case input:
    case button:
    case style:
    case meta:
        return true;

    default:
        return false;
    }
}

inline std::string to_string(TagType type)
{
    auto const index = std::to_underlying(type);
    if (index < 0 || index > std::to_underlying(TagType::_last_)) {
        return "_invalid_";
    }
    return tag_type_names_v[index];
}


using AttrValue = std::variant<
    std::monostate,
    int,
    std::string
>;

class Tag;

using TagContent = boost::variant<Tag, std::string>;


class Tag
{
public:
    using callback_type = std::function<http::status (boost::urls::params_view const&)>;

    Tag() = default;
    Tag(Tag const&) = default;
    Tag(Tag&&) = default;
    Tag& operator=(Tag const&) = default;
    Tag& operator=(Tag&&) = default;

    /*explicit*/ Tag(TagType type)
        : type_(type)
    {}

    TagType type() const noexcept { return type_; }
    bool is_void_element() const noexcept { return html::is_void_element(type_); }

    auto const& attrs() const noexcept { return attrs_; }
    auto& attrs() noexcept { return attrs_; }

    callback_type& callback() noexcept { return callback_; }

    std::string str() const;

    // ---------------------------------------

    std::vector<TagContent> const& contents() const noexcept { return contents_; }
    std::vector<TagContent>& contents() noexcept { return contents_; }

    template<class... Args>
    void append_string_content(Args&&... args)
    {
        contents_.emplace_back(std::string{std::forward<Args>(args)...});
    }

    auto children(this auto&& self, std::string_view attr_key, std::string_view attr_value)
    {
        return self.contents_
            | std::views::filter([&](auto const& tag_content) {
                return yk::visit(
                    yk::overloaded{
                        [](std::string const&) {
                            return false;
                        },
                        [&](Tag const& tag) {
                            auto it = tag.attrs().find(attr_key);
                            if (it == tag.attrs().end()) return false;

                            if (auto* v = std::get_if<std::string>(&it->second)) {
                                return *v == attr_value;
                            }
                            return false;
                        },
                    },
                    tag_content
                );
            }
        );
    }

    auto first_child(this auto&& self, std::string_view attr_key, std::string_view attr_value)
    {
        auto childs = self.children(attr_key, attr_value);
        auto it = childs.begin();
        if (it != childs.end()) {
            return yk::get<Tag>(&*it);
        }
        return static_cast<Tag*>(nullptr);
    }

private:
    TagType type_ = TagType::div;
    std::unordered_map<std::string, AttrValue, string_hash, std::equal_to<>> attrs_;
    std::vector<TagContent> contents_;

    callback_type callback_;
};

}

#endif
