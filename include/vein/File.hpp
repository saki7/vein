#ifndef VEIN_FILE_HPP
#define VEIN_FILE_HPP

#include "vein/LibraryConfig.hpp"

#include <boost/locale/conversion.hpp>
#include <boost/locale/generator.hpp>
#include <boost/locale/util.hpp>

#include <filesystem>
#include <string_view>


namespace vein {

struct [[nodiscard]] MIME
{
    std::string_view type;
    bool is_already_compressed = false;
};

inline MIME mime_type(std::filesystem::path const& path)
{
    boost::locale::generator gen;
    gen.locale_cache_enabled(true);
    auto const loc = gen(boost::locale::util::get_system_locale());
    auto const ext = boost::locale::to_lower(path.extension().string(), loc);

    if (ext == ".htm")  return {"text/html; charset=utf-8"};
    if (ext == ".html") return {"text/html; charset=utf-8"};
    if (ext == ".php")  return {"text/html; charset=utf-8"};
    if (ext == ".css")  return {"text/css; charset=utf-8"};
    if (ext == ".txt")  return {"text/plain; charset=utf-8"};
    if (ext == ".js")   return {"application/javascript; charset=utf-8"};
    if (ext == ".json") return {"application/json; charset=utf-8"};
    if (ext == ".xml")  return {"application/xml; charset=utf-8"};
    if (ext == ".flv")  return {"video/x-flv", true};
    if (ext == ".png")  return {"image/png", true};
    if (ext == ".jpe")  return {"image/jpeg", true};
    if (ext == ".jpeg") return {"image/jpeg", true};
    if (ext == ".jpg")  return {"image/jpeg", true};
    if (ext == ".gif")  return {"image/gif", true};
    if (ext == ".bmp")  return {"image/bmp"};
    if (ext == ".ico")  return {"image/vnd.microsoft.icon", true};
    if (ext == ".tiff") return {"image/tiff", true};
    if (ext == ".tif")  return {"image/tiff", true};
    if (ext == ".svg")  return {"image/svg+xml"};
    if (ext == ".svgz") return {"image/svg+xml"};
    return {"application/text"};
}

}

#endif
