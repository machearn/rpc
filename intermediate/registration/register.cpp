#include "include/register.hpp"

namespace mrpc {
std::int32_t Register::registerr(const std::string& json) {
    std::int32_t ret;
    ret = operation.insert(registry, json);
    return ret;
}

std::vector<std::tuple<std::string, std::int32_t>> Register::query(const std::string& func_name) {
    return operation.query(registry, func_name);
}

std::optional<std::int32_t> Register::update(const std::string& func_name, const std::string& json) {
    return operation.update(registry, func_name, json);
}

std::optional<std::int32_t> Register::drop(const std::string& func_name) {
    return operation.drop(registry, func_name);
}
} // namespace mrpc
