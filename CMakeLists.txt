cmake_minimum_required(VERSION 3.24)

project(vein VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "-std=c++2b")
set(CMAKE_CXX20_EXTENSION_COMPILE_OPTION "-std=gnu++2b")
set(CMAKE_CXX23_STANDARD_COMPILE_OPTION "-std=c++2b")
set(CMAKE_CXX23_EXTENSION_COMPILE_OPTION "-std=gnu++2b")
set(CMAKE_CXX_EXTENSIONS OFF)


add_library(vein STATIC)

target_sources(
    vein
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS include
        FILES
            include/vein/Controller.hpp
            include/vein/Error.hpp
            include/vein/File.hpp
            include/vein/HTTPSession.hpp
            include/vein/LibraryConfig.hpp
            include/vein/Listener.hpp
            include/vein/Router.hpp
            include/vein/Server.hpp
            include/vein/WebSocketSession.hpp
            include/vein/html/Builder.hpp
            include/vein/html/Document.hpp
            include/vein/html/Tag.hpp
            include/vein/html/Template.hpp
    PRIVATE
        FILE_SET private_headers
        TYPE HEADERS
        BASE_DIRS src
        FILES src/pch.cpp
    PRIVATE
        src/Controller.cpp
        src/HTTPSession.cpp
        src/Listener.cpp
        src/Router.cpp
        src/Server.cpp
        src/html/Tag.cpp
        src/html/Template.cpp
        src/pch.cpp
)

target_precompile_headers(
    vein
    PRIVATE
        src/pch.h
)

target_compile_definitions(
    vein
    PUBLIC
    BOOST_STATIC_STRING_STANDALONE
    BOOST_ALL_DYN_LINK
    BOOST_OPTIONAL_USE_SINGLETON_DEFINITION_OF_NONE
    SPDLOG_USE_STD_FORMAT
    SPDLOG_COMPILED_LIB
    U_CHARSET_IS_UTF8=1
    UNISTR_FROM_CHAR_EXPLICIT=explicit
    U_USING_ICU_NAMESPACE=0
    U_NO_DEFAULT_INCLUDE_UTF_HEADERS=1
)

add_library(SG14 INTERFACE)
target_sources(
    SG14
    INTERFACE
        FILE_SET HEADERS
        BASE_DIRS modules/SG14/include
        FILES modules/SG14/inplace_vector.h
)

add_subdirectory(modules/variant_view)
add_subdirectory(modules/yk_util)

target_link_libraries(vein PUBLIC SG14 yk::variant_view yk::yk_util)
