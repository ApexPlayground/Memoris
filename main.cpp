#include "Server.h"

#include <charconv>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string_view>

int main(int argc, char *argv[]) {
    try {
        if (argc > 2) throw std::invalid_argument("usage: ./memoris [port]");
        unsigned int port = 6379;
        if (argc == 2) {
            const std::string_view text(argv[1]);
            const auto result = std::from_chars(text.data(), text.data() + text.size(), port);
            if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
                port == 0 || port > 65535)
                throw std::invalid_argument("port must be an integer from 1 to 65535");
        }
        //
        if (std::signal(SIGPIPE, SIG_IGN) == SIG_ERR)
            throw std::runtime_error("could not ignore SIGPIPE");
        Server server(static_cast<std::uint16_t>(port));
        server.run();
    } catch (const std::exception &error) {
        std::cerr << "memoris: " << error.what() << '\n';
        return 1;
    }
}
