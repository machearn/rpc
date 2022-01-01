#include "include/database.hpp"

namespace mrpc {
DatabaseAccess &DatabaseAccess::instance() {
    static DatabaseAccess instance;
    return instance;
}

void DatabaseAccess::configure(std::unique_ptr<mongocxx::instance> instance,
                               std::unique_ptr<mongocxx::pool> pool) {
    _instance = std::move(instance);
    _pool = std::move(pool);
}

DatabaseAccess::connection DatabaseAccess::get_connection() {
    return _pool->acquire();
}

mongocxx::stdx::optional<DatabaseAccess::connection> DatabaseAccess::try_connection() {
    return _pool->try_acquire();
}


void configure(mongocxx::uri uri) {
    DatabaseAccess::instance().configure(std::make_unique<mongocxx::instance>(),
                                         std::make_unique<mongocxx::pool>(std::move(uri)));
}

DatabaseOperation::DatabaseOperation(const std::string &database) {
    auto connection = DatabaseAccess::instance().get_connection();
    db = std::make_unique<mongocxx::database>((*connection)[database]);
}

void DatabaseOperation::insert(const std::string &collection, const std::string &json) {
    auto coll = (*db)[collection];
    auto doc = bsoncxx::from_json(json);
    coll.insert_one(doc.view());
}

std::optional<std::string> DatabaseOperation::query(const std::string &collection, const std::string &name) {
    auto query_doc = mrpc::make_document(mrpc::kvp("name", name));
    auto coll = (*db)[collection];

    auto result = coll.find_one(query_doc.view());
    std::optional<std::string> url;
    if (result) {
        auto view = result->view();
        auto ip = view["ip"].get_utf8().value.to_string();
        auto port = view["port"].get_utf8().value.to_string();
        url = ip + ':' + port;
    }
    return url;
}
}