#include "include/database.hpp"

namespace mrpc {
DatabaseAccess& DatabaseAccess::instance() {
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

bool hasInstance(DatabaseAccess& obj) {
    return !(obj._instance == nullptr);
}


void configure(mongocxx::uri uri) {
    if (!hasInstance(DatabaseAccess::instance()))
        DatabaseAccess::instance().configure(std::make_unique<mongocxx::instance>(),
                                             std::make_unique<mongocxx::pool>(std::move(uri)));
}

DatabaseOperation::DatabaseOperation(const std::string& database) {
    auto connection = DatabaseAccess::instance().get_connection();
    db = std::make_unique<mongocxx::database>((*connection)[database]);
}

std::int32_t DatabaseOperation::insert(const std::string& collection, const std::string& json) {
    using view_or_value = bsoncxx::view_or_value<bsoncxx::document::view, bsoncxx::document::value>;
    view_or_value filter = bsoncxx::from_json(json);
    view_or_value doc = bsoncxx::document::value(make_document(kvp("$set", filter)));
    auto coll = (*db)[collection];

    auto options = mongocxx::options::update();
    auto result = coll.update_one(filter, doc, options.upsert(true));

    std::int32_t ret = result->matched_count();
    return ret;
}

std::vector<std::tuple<std::string, std::int32_t>>
DatabaseOperation::query(const std::string& collection, const std::string& name) {
    auto coll = (*db)[collection];

    auto cursor = coll.find(make_document(kvp("func_name", name)));
    std::vector<std::tuple<std::string, std::int32_t>> urls{};
    for (auto& doc: cursor)
        urls.emplace_back(doc["ip"].get_utf8().value.to_string(), doc["port"].get_int32().value);
    return urls;
}

std::optional<std::int32_t>
DatabaseOperation::update(const std::string& collection,
                          const std::string& origin,
                          const std::string& replace) {
    auto coll = (*db)[collection];
    auto result = coll.update_one(bsoncxx::from_json(origin), bsoncxx::from_json(replace));

    std::optional<std::int32_t> ret;
    if (result) ret = result->modified_count();
    return ret;
}

std::optional<std::int32_t>
DatabaseOperation::drop(const std::string& collection, const std::string& json) {
    auto coll = (*db)[collection];
    auto result = coll.delete_one(bsoncxx::from_json(json));

    std::optional<std::int32_t> ret;
    if (result) {
        ret = result->deleted_count();
    }
    return ret;
}

void dropDatabase(DatabaseOperation& operation) {
    operation.db->drop();
}
}// namespace mrpc