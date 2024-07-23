#include "pch.h"

#include "vein/Router.hpp"
#include "vein/Controller.hpp"


namespace vein {

Router::Router(std::string const& doc_root)
    : doc_root_(doc_root)
{}

Router::~Router() = default;

void Router::route(PathMatcher matcher, std::unique_ptr<Controller> controller)
{
    controllers_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(std::move(matcher)),
        std::forward_as_tuple(std::move(controller))
    );
}

}
