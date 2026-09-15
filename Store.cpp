#include "Store.h"
#include <utility>

void Store::set(std::string key, std::string value) {
    data_.insert_or_assign(std::move(key), std::move(value));
}

std::optional<std::string> Store::get(const std::string &key) const {
    const auto it = data_.find(key);
    if (it == data_.end()) return std::nullopt;
    return it->second;
}

bool Store::erase(const std::string &key) {
    return data_.erase(key) != 0;
}
