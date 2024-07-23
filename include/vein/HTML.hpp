#ifndef VEIN_HTML_HPP
#define VEIN_HTML_HPP

#include "vein/LibraryConfig.hpp"

#include <boost/url/params_view.hpp>

#include <boost/variant/variant.hpp>

#include <functional>
#include <memory>
#include <variant>
#include <string>
#include <vector>
#include <unordered_map>
#include <array>


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
    body,
    link,
    style,
    meta,
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
    "body",
    "link",
    "style",
    "meta",
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

using TagContent = std::variant<Tag, std::string>;


class Tag
{
public:
    using callback_type = std::move_only_function<void (boost::urls::params_view const&)>;

    Tag() = default;


    /*explicit*/ Tag(TagType type)
        : type_(type)
    {}

    TagType type() const noexcept { return type_; }
    bool is_void_element() const noexcept { return html::is_void_element(type_); }

    auto const& attrs() const noexcept { return attrs_; }
    auto& attrs() noexcept { return attrs_; }

    std::string str() const;

    std::vector<std::unique_ptr<TagContent>> const& contents() const noexcept { return contents_; }
    std::vector<std::unique_ptr<TagContent>>& contents() noexcept { return contents_; }

    callback_type& callback() noexcept { return callback_; }

private:
    TagType type_ = TagType::div;
    std::unordered_map<std::string, AttrValue> attrs_;
    std::vector<std::unique_ptr<TagContent>> contents_;

    callback_type callback_;
};

struct Document
{
    std::unordered_map<std::string, Tag*> name_tag, id_tag, form_action_tag;
};

}

#endif
