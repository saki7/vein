#ifndef VEIN_ERROR_HPP
#define VEIN_ERROR_HPP

#include "vein/LibraryConfig.hpp"

#include <boost/beast/core/error.hpp>

#include <iostream>


namespace vein {

namespace beast = boost::beast;

inline void fail(beast::error_code const& ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

}

#endif
