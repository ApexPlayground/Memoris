#pragma once

#include <optional>
#include <string>
#include <unordered_map>

// Owned by the single-threaded server; no synchronization is needed.
class Store {
public:
  void set(std::string key, std::string value);
  [[nodiscard]] std::optional<std::string> get(const std::string& key) const;
  bool erase(const std::string& key);

private:
  std::unordered_map<std::string, std::string> data_;
};
