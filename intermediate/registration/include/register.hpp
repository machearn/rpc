#ifndef REGISTER_H_
#define REGISTER_H_

#include "database.hpp"

namespace mrpc {

const std::int32_t REGISTER_THRESHOLD = 50;

class Register {
private:
    DatabaseOperation operation;
    std::string registry;
    std::int32_t threshold;

    Register() : operation("registration"), registry("registry"), threshold(REGISTER_THRESHOLD) {}
    ~Register() = default;
    explicit Register(const std::string& database)
            : operation(database), registry("registry"), threshold(REGISTER_THRESHOLD) {}
    explicit Register(DatabaseOperation&& operation_)
            : operation(std::forward<DatabaseOperation>(operation_)), registry("registry"), threshold(REGISTER_THRESHOLD) {}
public:
    Register(const Register&)=delete;
    Register(const Register&&)=delete;
    Register& operator=(const Register&)=delete;
    Register& operator=(const Register&&)=delete;
    static Register& instance() {
        static Register aRegister{};
        return aRegister;
    }

    std::int32_t add(const std::string&);
    std::vector<std::tuple<std::string, std::int32_t>> query(const std::string&);
    std::optional<std::int32_t> update(const std::string&, const std::string&);
    std::optional<std::int32_t> drop(const std::string&);
};
}// namespace mrpc

#endif// REGISTER_H_