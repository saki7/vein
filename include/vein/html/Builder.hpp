#ifndef VEIN_HTML_BUILDER_HPP
#define VEIN_HTML_BUILDER_HPP

#include "vein/html/Tag.hpp"

#include <yk/util/forward_like.hpp>

#include <charconv>
#include <array>


namespace vein::html {

template<TagType type>
class PredefBuilder;

template<class T>
struct make_tag_content
{
    template<class U>
    static void apply(Tag& tag, U&& value)
    {
        if constexpr (std::is_constructible_v<std::string, T>) {
            tag.contents().emplace_back(std::in_place_type<std::string>, std::forward<U>(value));

        } else if constexpr (std::integral<T>) {
            std::array<char, 16> buf;
            auto const [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), value);
            if (ec != std::errc{}) {
                tag.contents().emplace_back(std::in_place_type<std::string>, "(不正な値)");
                return;
            }
            tag.contents().emplace_back(std::in_place_type<std::string>, buf.data(), ptr);

        } else {
            tag.contents().emplace_back(std::in_place_type<T>, std::forward<U>(value));
        }
    }
};

template<TagType type>
struct make_tag_content<PredefBuilder<type>>
{
    template<class T>
    static void apply(Tag& tag, T&& predef_builder)
    {
        tag.contents().emplace_back(std::make_unique<Tag>(yk::forward_like<T>(predef_builder.tag_)));
    }
};

template<TagType type>
class PredefBuilder
{
    template<class T>
    friend struct make_tag_content;

public:
    PredefBuilder() = default;

    template<class... Builders>
        requires(sizeof...(Builders) >= 1)
    PredefBuilder(Builders&&... builders)
        : tag_{type}
    {
        (*this)(std::forward<Builders>(builders)...);
    }

    template<class Self, class K>
    decltype(auto) attr(this Self&& self, K&& k)
    {
        self.tag_.attrs().try_emplace(std::forward<K>(k));
        return std::forward<Self>(self);
    }

    template<class Self, class K, class V>
    decltype(auto) attr(this Self&& self, K&& k, V&& v)
    {
        self.tag_.attrs().emplace(
            std::piecewise_construct,
            std::forward_as_tuple(std::forward<K>(k)),
            std::forward_as_tuple(std::forward<V>(v))
        );
        return std::forward<Self>(self);
    }

    template<class Self, class V>
    decltype(auto) id(this Self&& self, V&& v)
    {
        return yk::forward_like<Self>(self.attr("id", std::forward<V>(v)));
    }
    template<class Self, class V>
    decltype(auto) klass(this Self&& self, V&& v)
    {
        return yk::forward_like<Self>(self.attr("class", std::forward<V>(v)));
    }

    template<class Self, class F>
    decltype(auto) callback(this Self&& self, F&& f)
    {
        self.tag_.callback() = std::forward<F>(f);
        return std::forward<Self>(self);
    }

    template<class Self>
    decltype(auto) operator()(this Self&& self)
    {
        return std::forward<Self>(self);
    }

    template<class Self, class... Builders>
    decltype(auto) operator()(this Self&& self, Builders&&... builders)
    {
        (make_tag_content<std::remove_cvref_t<Builders>>::apply(self.tag_, std::forward<Builders>(builders)), ...);
        return std::forward<Self>(self);
    }

    operator std::unique_ptr<Tag>()
    {
        return std::make_unique<Tag>(std::move(tag_));
    }

private:
    Tag tag_{type};
};

namespace builders {

using html::Tag;
using html::TagPtr;
using html::TagType;
using html::ClassList;

using section = PredefBuilder<TagType::section>;
using aside   = PredefBuilder<TagType::aside>;
using article = PredefBuilder<TagType::article>;
using header  = PredefBuilder<TagType::header>;
using footer  = PredefBuilder<TagType::footer>;
using menu    = PredefBuilder<TagType::menu>;
using nav     = PredefBuilder<TagType::nav>;

using table   = PredefBuilder<TagType::table>;
using thead   = PredefBuilder<TagType::thead>;
using tbody   = PredefBuilder<TagType::tbody>;
using tr      = PredefBuilder<TagType::tr>;
using th      = PredefBuilder<TagType::th>;
using td      = PredefBuilder<TagType::td>;
using caption = PredefBuilder<TagType::caption>;

using div    = PredefBuilder<TagType::div>;
using p      = PredefBuilder<TagType::p>;
using span   = PredefBuilder<TagType::span>;
using a      = PredefBuilder<TagType::a>;
using br     = PredefBuilder<TagType::br>;

using ul     = PredefBuilder<TagType::ul>;
using ol     = PredefBuilder<TagType::ol>;
using li     = PredefBuilder<TagType::li>;

using form   = PredefBuilder<TagType::form>;
using input  = PredefBuilder<TagType::input>;
using button = PredefBuilder<TagType::button>;
using main   = PredefBuilder<TagType::main>;
using body   = PredefBuilder<TagType::body>;
using link   = PredefBuilder<TagType::link>;
using style  = PredefBuilder<TagType::style>;
using script = PredefBuilder<TagType::script>;
using meta   = PredefBuilder<TagType::meta>;
using title  = PredefBuilder<TagType::title>;
using head   = PredefBuilder<TagType::head>;
using html   = PredefBuilder<TagType::html>;

} // builders

}

#endif
