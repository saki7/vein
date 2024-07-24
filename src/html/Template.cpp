#include "pch.h"

#include "vein/html/Template.hpp"

#include <fstream>


namespace vein::html {

std::string const& TemplateLoader::partial(std::string const& rel_path)
{
    auto path = std::filesystem::path(rel_path);

    if (!path.filename().string().starts_with('_')) {
        throw std::invalid_argument{"partial template file name does not start with underscore"};
    }
    path.replace_extension("html");
    path = canonical(root_ / path);

    if (auto const it = partial_cache_.find(path); it != partial_cache_.end()) {
        return it->second;
    }
    if (!exists(path)) {
        throw std::invalid_argument{"partial template does not exist on filesystem: " + path.string()};
    }


    std::string buf;
    buf.resize(file_size(path));

    {
        std::ifstream ifs{path, std::ios::in | std::ios::binary};
        ifs.read(buf.data(), buf.size());
    }

    auto [kv, _] = partial_cache_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(path),
        std::forward_as_tuple(std::move(buf))
    );
    return kv->second;
}

}
