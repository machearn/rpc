#ifndef DATABASE_H_
#define DATABASE_H_

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <tuple>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

namespace mrpc {
class DatabaseAccess {
public:
    static DatabaseAccess& instance();

    void configure(std::unique_ptr<mongocxx::instance> instance,
                   std::unique_ptr<mongocxx::pool> pool);

    using connection = mongocxx::pool::entry;

    connection get_connection();

    mongocxx::stdx::optional<connection> try_connection();
    friend bool hasInstance(DatabaseAccess&);

private:
    DatabaseAccess() = default;

    std::unique_ptr<mongocxx::instance> _instance = nullptr;
    std::unique_ptr<mongocxx::pool> _pool = nullptr;
};

void configure(mongocxx::uri);

class DatabaseOperation {
public:
    DatabaseOperation() = default;

    explicit DatabaseOperation(const std::string&);

    std::optional<std::string> insert(const std::string&, const std::string&);

    std::optional<std::tuple<std::string, std::int32_t>> query(const std::string&,
                                                               const std::string&);

    std::optional<std::int32_t> update(const std::string&, const std::string&, const std::string&);

    std::optional<std::int32_t> drop(const std::string&, const std::string&);

    friend void dropDatabase(DatabaseOperation&);

private:
    std::unique_ptr<mongocxx::database> db = nullptr;
};
}// namespace mrpc

#endif// DATABASE_H_
