#include "database.hpp"

#include <gtest/gtest.h>

class DatabaseTest : public ::testing::Test {
protected:
    mrpc::DatabaseOperation operation;
    std::string collection;

    void SetUp() override {
        mongocxx::uri uri{};
        mrpc::configure(std::move(uri));
        operation = mrpc::DatabaseOperation{"test"};
        collection = "register";
    }
};

TEST_F(DatabaseTest, InsertTestSuccess) {
    auto result = operation.insert(collection,
                                   R"({"func_name": "abc::test", "ip": "123.123.123.123", "port": 89009})");
    EXPECT_TRUE(result);
}

TEST_F(DatabaseTest, QueryTestAfterInsert) {
    auto result = operation.query(collection, "abc::test");
    EXPECT_TRUE(result);
    EXPECT_EQ("123.123.123.123", std::get<std::string>(*result));
    EXPECT_EQ(89009, std::get<std::int32_t>(*result));
}

TEST_F(DatabaseTest, InsertTestFail) {
    auto result = operation.insert(collection,
                                   R"({"func_name": "abc::test", "ip": "123.123.123.123", "port": 89009})");
    EXPECT_FALSE(result);
}

TEST_F(DatabaseTest, UpdateTestSuccess) {
    auto result = operation.update(collection, "abc::test",
                                   R"({"$set": {"ip": "23.23.23.23", "port": 89109}})");
    EXPECT_EQ(1, *result);
}

TEST_F(DatabaseTest, QueryTestAfterUpdate) {
    auto result = operation.query(collection, "abc::test");
    EXPECT_TRUE(result);
    EXPECT_EQ("23.23.23.23", std::get<std::string>(*result));
    EXPECT_EQ(89109, std::get<std::int32_t>(*result));
}

TEST_F(DatabaseTest, UpdateTestFail) {
    auto result = operation.update(collection, "bac::test",
                                   R"({"$set": {"ip": "23.23.23.23", "port": 89109}})");
    EXPECT_EQ(0, *result);
}

TEST_F(DatabaseTest, DropTestFail) {
    auto result = operation.drop(collection, "bac::test");
    EXPECT_EQ(0, *result);
}

TEST_F(DatabaseTest, DropTestSuccess) {
    auto result = operation.drop(collection, "abc::test");
    EXPECT_EQ(1, *result);
}

TEST_F(DatabaseTest, QueryTestAfterDrop) {
    auto result = operation.query(collection, "abc::test");
    EXPECT_FALSE(result);
}

TEST_F(DatabaseTest, DropDatabaseTest) {
    dropDatabase(operation);
}
