# vein
Minimal MVC Web Framework for C++


## Example

### MainController.hpp

```cpp
#include <vein/Controller.hpp>
#include <string>

class MainController : public vein::CustomController<MainController>
{
public:
    MainController()
    {
        using namespace vein::html::builders;
        using vein::html::builders::div; // for avoiding standard `div` symbol
        using vein::html::builders::main; // for avoiding standard `main` symbol

        this->set_html(html{}.attr("lang", "ja")(
            head{
                meta{}.attr("charset", "UTF-8"),
                meta{}
                    .attr("name", "viewport")
                    .attr("content", "width=device-width, initial-scale=1, viewport-fit=cover"),
                title{"Default title"},
                script{}.attr("src", "/js/index.js").attr("defer"),
                link{}.attr("rel", "stylesheet").attr("href", "/css/index.css"),
            },
            "\n",
            body{}.klass("page-main")(
                main{}.id("main")(
                    "Default body"
                )
            )
        ));

        this->set_default_callback([this](boost::urls::url_view const& url) {
            this->set_title("Hello, world!");

            auto* main_tag = this->tag_by_id("main");
            main_tag->contents().clear();

            TagPtr content_tag = div{}(
                "Hello, world!",
                br{},
                std::string{"url: "} + url.c_str()
            );
            main_tag->contents().emplace_back(std::move(content_tag));

            return boost::beast::http::status::ok;
        });
    }
};
```

### main.cpp

```cpp
#include "MainController.hpp"

#include <vein/Server.hpp>
#include <vein/Router.hpp>

#include <boost/url/url.hpp>

#include <iostream>
#include <filesystem>
#include <string>
#include <memory>
#include <utility>

int main(int argc, char* argv[])
{
    if (argc < 6) {
        std::cerr << "invalid arguments" << std::endl;
    }

    std::filesystem::path const public_root{argv[1]};
    boost::urls::url const canonical_url_origin{argv[2]};

    vein::Server server;
    auto router = std::make_unique<vein::Router>(public_root);

    auto controller = std::make_unique<MainController>();
    controller->set_canonical_url_origin(canonical_url_origin);
    router->route("/", std::move(controller));

    return server.wait(
        argv[3], // host
        std::stoi(argv[4]), // port
        std::move(router),
        std::stoul(argv[5]) // worker_thread_count
    );
}
```

### Usage

```console
$ ./main [public_root] [canonical_url_origin] [host] [port] [worker_thread_count]
```

| Argument | Description |
|---|---|
| `public_root` | The root directory for serving html/css/js/etc. assets |
| `canonical_url_origin` | The origin prefix, without trailing slash, for generating `<link rel="canonical" href="...">` <br>e.g. `http://localhost:8080` or `http://example.com` |
| `host` | TCP listening host |
| `port` | TCP listening port |
| `worker_thread_count` | Worker thread count (this will set the internal count of `boost::asio::io_context`)
