#ifndef REGISTER_H_
#define REGISTER_H_

#include "database.hpp"

namespace mrpc {
class Register {
private:
    DatabaseOperation operation;
    std::string registry;

public:
    Register() : operation("registration"), registry("registry") {}
    explicit Register(const std::string& database)
        : operation(database), registry("registry") {}
    explicit Register(DatabaseOperation&& operation_)
        : operation(std::forward<DatabaseOperation>(operation_)), registry("registry") {}
    ~Register() = default;

    std::optional<std::string> registerr(const std::string&);
    std::optional<std::tuple<std::string, std::int16_t>> query(const std::string&);
    std::optional<std::int32_t> update(const std::string&, const std::string&);
    std::optional<std::int32_t> drop(const std::string&);
};
}// namespace mrpc

#endif// REGISTER_H_