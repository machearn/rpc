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

    std::int32_t registerr(const std::string&);
    std::vector<std::tuple<std::string, std::int32_t>> query(const std::string&);
    std::optional<std::int32_t> update(const std::string&, const std::string&);
    std::optional<std::int32_t> drop(const std::string&);
};
}// namespace mrpc

#endif// REGISTER_H_