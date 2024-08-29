#ifndef VEIN_HTML_TAG_HPP
#define VEIN_HTML_TAG_HPP

#include "vein/LibraryConfig.hpp"

#include <yk/string_hash.hpp>
#include <yk/util/overloaded.hpp>
#include <yk/variant/std.hpp>

#include <boost/variant/variant.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/url/params_view.hpp>

#include <unordered_map>
#include <unordered_set>
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
    section,
    aside,
    article,

    header,
    footer,
    menu,
    nav,

    table,
    thead,
    tbody,
    tr,
    th,
    td,
    caption,

    div,
    p,
    span,
    a,
    br,

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
    "section",
    "aside",
    "article",

    "header",
    "footer",
    "menu",
    "nav",

    "table",
    "thead",
    "tbody",
    "tr",
    "th",
    "td",
    "caption",

    "div",
    "p",
    "span",
    "a",
    "br",

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

// https://html.spec.whatwg.org/multipage/syntax.html#void-elements
constexpr bool is_void_element(TagType type) noexcept
{
    using enum TagType;

    switch (type) {
    //case area:
    //case base:
    //case br:
    //case col:
    //case embed:
    //case hr:
    //case img:
    case input:
    case link:
    case meta:
    //case source:
    //case track:
    //case wbr:
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

using ClassList = std::unordered_set<std::string, yk::string_hash, std::equal_to<>>;

using AttrValue = std::variant<
    std::monostate,
    int,
    std::string,
    ClassList
>;

class Tag;
using TagPtr = std::unique_ptr<Tag>;

using TagContent = std::variant<TagPtr, std::string>;


class Tag
{
public:
    using callback_type = std::function<http::status (boost::urls::url_view const&)>;

    Tag() = default;
    Tag(Tag&&) = default;
    Tag& operator=(Tag const&) = default;
    Tag& operator=(Tag&&) = default;

    /*explicit*/ Tag(TagType type)
        : type_(type)
    {}

    Tag(Tag const& other)
        : type_(other.type_)
        , attrs_(other.attrs_)
        , callback_(other.callback_)
    {
        contents_.reserve(other.contents_.size());

        for (auto const& content : other.contents_) {
            contents_.emplace_back(std::visit<TagContent>(yk::overloaded{
                [](TagPtr const& tag_ptr) {
                    return std::make_unique<Tag>(*tag_ptr);
                },
                [](auto const& value) {
                    return value;
                },
            }, content));
        }
    }

    [[nodiscard]] TagType type() const noexcept { return type_; }
    [[nodiscard]] bool is_void_element() const noexcept { return html::is_void_element(type_); }

    [[nodiscard]] auto const& attrs() const noexcept { return attrs_; }
    [[nodiscard]] auto& attrs() noexcept { return attrs_; }

    [[nodiscard]] decltype(auto) classes() const noexcept { return std::get<ClassList>(attrs_.at("class")); }
    [[nodiscard]] decltype(auto) classes() noexcept { return std::get<ClassList>(attrs_.at("class")); }

    [[nodiscard]] bool matches(std::string_view attr, std::string_view value) const noexcept
    {
        auto const it = attrs_.find(attr);
        if (it == attrs_.end()) return false;
        auto* val = std::get_if<std::string>(&it->second);
        if (!val) return false;
        return *val == value;
    }

    [[nodiscard]] callback_type& callback() noexcept { return callback_; }

    [[nodiscard]] std::string str() const;

    // ---------------------------------------

    [[nodiscard]] std::vector<TagContent> const& contents() const noexcept { return contents_; }
    [[nodiscard]] std::vector<TagContent>& contents() noexcept { return contents_; }

    template<class... Args>
    void append_string_content(Args&&... args)
    {
        contents_.emplace_back(std::string{std::forward<Args>(args)...});
    }

    [[nodiscard]] auto children(this auto&& self, std::string_view attr_key, std::string_view attr_value) noexcept
    {
        return self.contents_
            | std::views::filter([attr_key = std::string{attr_key}, attr_value = std::string{attr_value}](auto const& tag_content) {
                return yk::visit(
                    yk::overloaded{
                        [](std::string const&) {
                            return false;
                        },
                        [&](TagPtr const& tag) {
                            auto it = tag->attrs().find(attr_key);
                            if (it == tag->attrs().end()) return false;

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

    [[nodiscard]] auto first_child(this auto&& self, TagType type) noexcept
    {
        for (auto const& child : self.contents_) {
            auto* tag_ptr = std::get_if<TagPtr>(&child);
            if (!tag_ptr) continue;
            if ((*tag_ptr)->type() == type) {
                return tag_ptr->get();
            }
        }
        return static_cast<Tag*>(nullptr);
    }

    [[nodiscard]] auto first_child(this auto&& self, std::string_view attr_key, std::string_view attr_value) noexcept
    {
        auto childs = self.children(attr_key, attr_value);
        auto it = childs.begin();
        if (it != childs.end()) {
            auto tag_ptr = yk::get<TagPtr>(&*it);
            if (!tag_ptr) return static_cast<Tag*>(nullptr);
            return tag_ptr->get();
        }
        return static_cast<Tag*>(nullptr);
    }

private:
    TagType type_ = TagType::div;
    std::unordered_map<std::string, AttrValue, yk::string_hash, std::equal_to<>> attrs_;
    std::vector<TagContent> contents_;

    callback_type callback_;
};

}

#endif
