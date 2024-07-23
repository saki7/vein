#ifndef VEIN_SERVER_HPP
#define VEIN_SERVER_HPP

#include "vein/LibraryConfig.hpp"

#include <string>
#include <memory>


namespace vein {

class Router;

class Server
{
public:
    [[nodiscard]] int wait(
        std::string const& host,
        unsigned port,
        std::unique_ptr<Router> router, 
        unsigned thread_count
    );

private:
};

}

#endif
