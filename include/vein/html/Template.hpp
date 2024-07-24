#ifndef VEIN_HTML_TEMPLATE_HPP
#define VEIN_HTML_TEMPLATE_HPP

#include "vein/LibraryConfig.hpp"

#include <filesystem>
#include <unordered_map>
#include <string>


namespace vein::html {

class TemplateLoader
{
public:
    explicit TemplateLoader(std::filesystem::path const& root)
        : root_(root)
    {}

    std::string const& partial(std::string const& rel_path);

private:
    std::filesystem::path root_;
    std::unordered_map<std::filesystem::path, std::string> partial_cache_;
};

}

#endif
