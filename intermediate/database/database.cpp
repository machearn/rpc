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

std::optional<std::string>
DatabaseOperation::insert(const std::string &collection, const std::string &json) {
    auto doc = bsoncxx::from_json(json).view();
    auto func_name = doc["name"].get_utf8().value.to_string();
    auto query_result = this->query(collection, func_name);
    std::optional<std::string> ret;
    if (!query_result) {
        auto coll = (*db)[collection];
        auto insert_result = coll.insert_one(doc);
        if (insert_result) {
            ret = insert_result->inserted_id().get_oid().value.to_string();
        }
    }
    return ret;
}

std::optional<std::tuple<std::string, std::int64_t>>
DatabaseOperation::query(const std::string &collection, const std::string &name) {
    auto query_doc = mrpc::make_document(mrpc::kvp("name", name));
    auto coll = (*db)[collection];

    auto result = coll.find_one(query_doc.view());
    std::optional<std::tuple<std::string, std::int64_t>> url;
    if (result) {
        auto view = result->view();
        auto ip = view["ip"].get_utf8().value.to_string();
        auto port = view["port"].get_int64().value;
        url = {ip, port};
    }
    return url;
}

std::optional<std::int32_t>
DatabaseOperation::update(const std::string &collection,
                          const std::string &name,
                          const std::string &json) {
    auto doc = bsoncxx::from_json(json).view();
    auto filter = mrpc::make_document(mrpc::kvp("name", name)).view();

    auto coll = (*db)[collection];
    auto result = coll.update_one(filter, doc);

    std::optional<std::int32_t> ret;
    if (result) {
        ret = result->modified_count();
    }

    return ret;
}

std::optional<std::int32_t>
DatabaseOperation::drop(const std::string &collection, const std::string &name) {
    auto filter = mrpc::make_document(mrpc::kvp("name", name)).view();
    auto coll = (*db)[collection];
    auto result = coll.delete_one(filter);

    std::optional<std::int32_t> ret;
    if (result) {
        ret = result->deleted_count();
    }
    return ret;
}
}