#ifndef DATABASE_H_
#define DATABASE_H_

#include <cstdint>
#include <iostream>
#include <vector>
#include <memory>
#include <optional>

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>

namespace mrpc {
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::kvp;

class DatabaseAccess {
public:
    static DatabaseAccess &instance();

    void configure(std::unique_ptr<mongocxx::instance> instance,
                   std::unique_ptr<mongocxx::pool> pool);

    using connection = mongocxx::pool::entry;

    connection get_connection();

    mongocxx::stdx::optional<connection> try_connection();

private:
    DatabaseAccess() = default;

    std::unique_ptr<mongocxx::instance> _instance = nullptr;
    std::unique_ptr<mongocxx::pool> _pool = nullptr;
};

void configure(mongocxx::uri);

class DatabaseOperation {
public:
    DatabaseOperation()=default;
    explicit DatabaseOperation(const std::string&);

    void insert(const std::string&, const std::string&);

    std::optional<std::string> query(const std::string&, const std::string&);

private:
    std::unique_ptr<mongocxx::database> db = nullptr;
};
}

#endif // DATABASE_H_
