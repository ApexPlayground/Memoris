#include "Server.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <iostream>
#include <sstream>
#include <system_error>
#include <utility>
#include <vector>

namespace {
    // Move-only ownership: every descriptor closes on normal return or exception.
    class Socket {
    public:
        explicit Socket(int fd) : fd_(fd) {
            if (fd < 0) throw std::system_error(errno, std::generic_category(), "socket");
        }

        ~Socket() { if (fd_ >= 0) ::close(fd_); }

        Socket(const Socket &) = delete;

        Socket &operator=(const Socket &) = delete;

        Socket(Socket &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {
        }

        Socket &operator=(Socket &&other) noexcept {
            if (this != &other) {
                if (fd_ >= 0) ::close(fd_);
                fd_ = std::exchange(other.fd_, -1);
            }
            return *this;
        }

        int get() const noexcept { return fd_; }

    private:
        int fd_;
    };

    void check(int result, const char *operation) {
        if (result < 0) throw std::system_error(errno, std::generic_category(), operation);
    }

    bool sendAll(int fd, const std::string &reply) {
        std::size_t sent = 0;
        while (sent < reply.size()) {
            const auto n = ::send(fd, reply.data() + sent, reply.size() - sent, 0);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) return false; // Disconnected clients must not stop the server.
            sent += static_cast<std::size_t>(n);
        }
        return true;
    }
} // namespace

void Server::run() {
    Socket listener(::socket(AF_INET, SOCK_STREAM, 0));
    const int reuse = 1;
    check(::setsockopt(listener.get(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)),
          "setsockopt");

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port_);
    check(::bind(listener.get(), reinterpret_cast<const sockaddr *>(&address), sizeof(address)),
          "bind");
    check(::listen(listener.get(), 16), "listen");
    std::cout << "memoris listening on 127.0.0.1:" << port_ << std::endl;

    for (;;) {
        const int fd = ::accept(listener.get(), nullptr, nullptr);
        if (fd < 0 && errno == EINTR) continue;
        check(fd, "accept");
        Socket client(fd);
        handleClient(client.get()); // serve one connection at a time.
    }
}

void Server::handleClient(int fd) {
    constexpr std::size_t maxLine = 4096; // Bytes before LF, including optional CR.
    std::array<char, 4096> buffer{};
    std::string pending;
    for (;;) {
        const auto n = ::recv(fd, buffer.data(), buffer.size(), 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return; // EOF discards an unfinished command.

        // TCP has no message boundaries: retain fragments and split coalesced lines.
        for (std::size_t i = 0; i < static_cast<std::size_t>(n); ++i) {
            if (buffer[i] == '\n') {
                if (!pending.empty() && pending.back() == '\r') pending.pop_back();
                if (!sendAll(fd, execute(pending))) return;
                pending.clear();
            } else {
                if (pending.size() == maxLine) {
                    sendAll(fd, "-ERR command too long\r\n");
                    return;
                }
                pending.push_back(buffer[i]);
            }
        }
    }
}

std::string Server::execute(const std::string &line) {
    std::istringstream input(line);
    std::vector<std::string> args;
    for (std::string token; input >> token;) args.push_back(std::move(token));
    if (args.empty()) return "-ERR empty command\r\n";
    // ASCII case-insensitive commands; keys and values retain their case.
    for (char &c: args[0]) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    const auto &command = args[0];
    if (command != "PING" && command != "SET" && command != "GET" && command != "DEL")
        return "-ERR unknown command\r\n";
    const std::size_t expected = command == "PING" ? 1 : command == "SET" ? 3 : 2;
    if (args.size() != expected) return "-ERR wrong number of arguments\r\n";
    if (command == "PING") return "+PONG\r\n";
    if (command == "SET") {
        store_.set(std::move(args[1]), std::move(args[2]));
        return "+OK\r\n";
    }
    if (command == "DEL") return store_.erase(args[1]) ? ":1\r\n" : ":0\r\n";
    const auto value = store_.get(args[1]);
    if (!value) return "$-1\r\n";
    return "$" + std::to_string(value->size()) + "\r\n" + *value + "\r\n";
}
