#include "include/register.hpp"

namespace mrpc {
std::int32_t Register::add(const std::string& json) {
    return operation.insert(registry, json);
}

std::vector<std::tuple<std::string, std::int32_t>> Register::query(const std::string& func_name) {
    return operation.query(registry, func_name);
}

std::optional<std::int32_t> Register::update(const std::string& origin, const std::string& replace) {
    return operation.update(registry, origin, replace);
}

std::optional<std::int32_t> Register::drop(const std::string& json) {
    return operation.drop(registry, json);
}
} // namespace mrpc
