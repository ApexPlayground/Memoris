#include "Store.h"
#include <utility>

void Store::set(const std::string& key, const std::string& value) {
    data_.insert_or_assign(key, value);
}

std::optional<std::string> Store::get(const std::string& key) const {
    const auto entry = data_.find(key);

    if (entry == data_.end()) {
        return std::nullopt; // Key not found.
    }
    return entry->second; // Return the stored value.
}

bool Store::erase(const std::string& key) {
    auto const removed = data_.erase(key);

    if (removed != 0) {
        return true;
    }
    return false;
}