#ifndef VEIN_LIBRARY_CONFIG_HPP
#define VEIN_LIBRARY_CONFIG_HPP

#include <SDKDDKVer.h>

#ifdef VEIN_EXPORTS
# define VEIN_API __declspec(dllexport)
#else
# define VEIN_API __declspec(dllimport)
#endif


namespace vein {



}

#endif
