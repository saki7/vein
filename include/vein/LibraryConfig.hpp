#ifndef VEIN_LIBRARY_CONFIG_HPP
#define VEIN_LIBRARY_CONFIG_HPP

#include <boost/config.hpp>

#if BOOST_WINDOWS
#include <SDKDDKVer.h>
#endif

#if VEIN_EXPORTS

# if BOOST_WINDOWS
#  define VEIN_API __declspec(dllexport)
# else
#  define VEIN_API __attribute__((visibility("default")))
# endif

#else // VEIN_EXPORTS == 0

# if BOOST_WINDOWS
#  define VEIN_API __declspec(dllimport)
# else
#  define VEIN_API
# endif

#endif // VEIN_EXPORTS


namespace vein {



}

#endif
