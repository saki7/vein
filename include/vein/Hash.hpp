#ifndef VEIN_HASH_HPP
#define VEIN_HASH_HPP

#include "vein/LibraryConfig.hpp"

#include <string>
#include <string_view>
#include <functional>


namespace vein {

template<class CharT, class TraitsT = std::char_traits<CharT>>
struct basic_string_hash
{
    using is_transparent = std::true_type;

    std::size_t operator()(std::basic_string_view<CharT, TraitsT> txt) const noexcept
    {
        return std::hash<std::basic_string_view<CharT, TraitsT>>{}(txt);
    }

    template<class AllocT>
    std::size_t operator()(std::basic_string<CharT, TraitsT, AllocT> const& txt) const noexcept
    {
        return std::hash<std::basic_string_view<CharT, TraitsT>>{}(std::basic_string_view<CharT, TraitsT>{txt});
    }

    std::size_t operator()(CharT const* txt) const noexcept
    {
        return std::hash<std::basic_string_view<CharT>>{}(std::basic_string_view<CharT>{txt});
    }
};

using string_hash = basic_string_hash<char>;
using wstring_hash = basic_string_hash<wchar_t>;

using u8string_hash = basic_string_hash<char8_t>;
using u16string_hash = basic_string_hash<char16_t>;
using u32string_hash = basic_string_hash<char32_t>;


template<class BaseHashT, auto Mem>
struct proxy_hash;

template<class BaseHashT, class T, class Klass, T Klass::* Mem>
struct proxy_hash<BaseHashT, Mem> : BaseHashT
{
    using BaseHashT::operator();

    std::size_t operator()(Klass const& klass) const
    {
        return (*this)(klass.*Mem);
    }
};

}

#endif
