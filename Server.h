#pragma once

#include "Store.h"
#include <cstdint>
#include <string>

class Server {
public:
    explicit Server(std::uint16_t port) : port_(port) {
    }

    void run();

private:
    void handleClient(int fd);

    std::string execute(const std::string &line);

    std::uint16_t port_;
    Store store_;
};
