#include "include/register.hpp"

namespace mrpc {
std::optional<std::string> Register::registerr(const std::string& json) {
    std::optional<std::string> ret;
    ret = operation.insert(registry, json);
    return ret;
}

std::optional<std::tuple<std::string, std::int16_t>> Register::query(const std::string& func_name) {
    return operation.query(registry, func_name);
}

std::optional<std::int32_t> Register::update(const std::string& func_name, const std::string& json) {
    return operation.update(registry, func_name, json);
}

std::optional<std::int32_t> Register::drop(const std::string& func_name) {
    return operation.drop(registry, func_name);
}
} // namespace mrpc
