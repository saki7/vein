#ifndef VEIN_HTML_BUILDER_HPP
#define VEIN_HTML_BUILDER_HPP

#include "vein/html/Tag.hpp"


namespace vein::html {

class Builder;

template<TagType type>
class PredefBuilder;


template<class T>
struct make_tag_content
{
    template<class T>
    static void apply(Tag& tag, T&& value)
    {
        if constexpr (std::is_convertible_v<T, std::string>) {
            tag.contents().emplace_back(std::make_unique<TagContent>(std::in_place_type<std::string>, std::forward<T>(value)));

        } else {
            tag.contents().emplace_back(std::make_unique<TagContent>(std::in_place_type<std::remove_cvref_t<T>>, std::forward<T>(value)));
        }
    }
};

template<>
struct make_tag_content<Builder>
{
    template<class T>
    static void apply(Tag& tag, T&& builder)
    {
        tag.contents().emplace_back(std::make_unique<TagContent>(std::in_place_type<Tag>, std::forward_like<T>(builder.tag_)));
    }
};

template<TagType type>
struct make_tag_content<PredefBuilder<type>>
{
    template<class T>
    static void apply(Tag& tag, T&& predef_builder)
    {
        tag.contents().emplace_back(std::make_unique<TagContent>(std::in_place_type<Tag>, std::forward_like<T>(predef_builder.builder_.tag_)));
    }
};

class Builder
{
    template<class T>
    friend struct make_tag_content;

public:
    template<class... Args>
    explicit Builder(Args&&... args)
        : tag_{std::forward<Args>(args)...}
    {
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
        return std::forward_like<Self>(self.attr("id", std::forward<V>(v)));
    }
    template<class Self, class V>
    decltype(auto) klass(this Self&& self, V&& v)
    {
        return std::forward_like<Self>(self.attr("class", std::forward<V>(v)));
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
    Tag tag_;
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
        : builder_{type}
    {
        builder_(std::forward<Builders>(builders)...);
    }

    template<class K, class V>
    Builder attr(K&& k, V&& v) const
    {
        Builder builder{type};
        builder.attr(std::forward<K>(k), std::forward<V>(v));
        return builder;
    }
    template<class K>
    Builder attr(K&& k) const
    {
        Builder builder{type};
        builder.attr(std::forward<K>(k));
        return builder;
    }

    template<class V>
    Builder id(V&& v) const
    {
        Builder builder{type};
        builder.id(std::forward<V>(v));
        return builder;
    }
    
    template<class V>
    Builder klass(V&& v) const
    {
        Builder builder{type};
        builder.klass(std::forward<V>(v));
        return builder;
    }

    operator Builder() const
    {
        return Builder{type};
    }

    template<class... Args>
    Builder operator()(Args&&... args) const
    {
        Builder builder{type};
        builder(std::forward<Args>(args)...);
        return builder;
    }

    operator std::unique_ptr<Tag>()
    {
        return builder_;
    }

private:
    Builder builder_;
};

namespace builders {

using div    = PredefBuilder<TagType::div>;
using p      = PredefBuilder<TagType::p>;
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
using head   = PredefBuilder<TagType::head>;
using html   = PredefBuilder<TagType::html>;

} // builders

}

#endif
