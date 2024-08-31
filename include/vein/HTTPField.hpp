#ifndef VEIN_HTTP_FIELD_HPP
#define VEIN_HTTP_FIELD_HPP

#include "vein/LibraryConfig.hpp"

#include <boost/beast/http/field.hpp>

#include <unordered_map>
#include <string>


namespace vein {

namespace beast = boost::beast;
namespace http = beast::http;

using HTTPFields = std::unordered_map<http::field, std::string>;

}

#endif
