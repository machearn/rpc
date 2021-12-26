#include "generator.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>
#include <map>

using json = nlohmann::json;
struct TestArg {
    int id;
    std::string name;

    TestArg(): id(0), name() {}
    TestArg(int id_, std::string name_): id(id_), name(std::move(name_)) {}
};

const json arg_rhs = R"({"id": 1, "name": "hello"})"_json;

void to_json(json& j, const TestArg& arg) {
    j["id"] = arg.id;
    j["name"] = arg.name;
}

void from_json(const json& j, TestArg& arg) {
    j["id"].get_to(arg.id);
    j["name"].get_to(arg.name);
}

bool arg_json_eq(const json& lhs, const json& rhs) {
    if (lhs["id"] == rhs["id"] && lhs["name"] == rhs["name"])
        return true;
    return false;
}

TEST(ToJsonTest, IntToJson) {
    json j;
    mrpc::to_json(j, 1);
    auto dump = j.get<int>();
    EXPECT_EQ(dump, 1);
}

TEST(ToJsonTest, StringToJson) {
    json j_string;
    json j_const_string;
    std::string s("hello world");
    mrpc::to_json(j_string, s);
    mrpc::to_json(j_const_string, "hello world");
    auto dump_string = j_string.get<std::string>();
    auto dump_const_string = j_const_string.get<std::string>();
    EXPECT_EQ(dump_string, s);
    EXPECT_EQ(dump_const_string, "hello world");
}

TEST(ToJsonTest, ObjectToJson) {
    json lhs;
    TestArg arg{1, "hello"};
    mrpc::to_json(lhs, arg);
    json rhs_ = R"({"id": 2, "name": "hello"})"_json;
    EXPECT_TRUE(arg_json_eq(lhs, arg_rhs));
    EXPECT_FALSE(arg_json_eq(lhs, rhs_));
}

TEST(ToJsonTest, VectorToJson) {
    json lhs;
    std::vector<TestArg> args{};
    args.emplace_back(1, "hello");
    json arg1 = R"({"id": 1, "name": "hello"})"_json;
    args.emplace_back(2, "world");
    json arg2 = R"({"id": 2, "name": "world"})"_json;
    json rhs;
    rhs[0] = arg1;
    rhs[1] = arg2;
    mrpc::to_json(lhs, args);
    for (int i = 0; i < 2; i++) {
        EXPECT_TRUE(arg_json_eq(lhs[i], rhs[i]));
    }
}

TEST(ToJsonTest, MapToJson) {
    json lhs;
    json rhs;
    std::map<std::string, TestArg> kwargs{};
    kwargs.emplace(std::make_pair("one", TestArg{1, "hello"}));
    rhs["one"] = R"({"id": 1, "name": "hello"})"_json;
    kwargs.emplace(std::make_pair("two", TestArg{2, "world"}));
    rhs["two"] = R"({"id": 2, "name": "world"})"_json;

    mrpc::to_json(lhs, kwargs);
    for (auto &p : kwargs) {
        EXPECT_TRUE(arg_json_eq(lhs[p.first], rhs[p.first]));
    }
}

TEST(AddTypeTest, ObjectInteger) {
    json integer_object;
    mrpc::add_type(integer_object, 1);
    EXPECT_EQ(integer_object["type"], "object");
    EXPECT_EQ(integer_object["arg"], 1);
}

TEST(AddTypeTest, ObjectClass) {
    json class_object;
    TestArg arg{1, "hello"};
    mrpc::add_type(class_object, arg);
    EXPECT_EQ(class_object["type"], "object");
    EXPECT_TRUE(arg_json_eq(class_object["arg"], arg_rhs));
}

TEST(AddTypeTest, Pointer) {
    json pointer;
    auto* arg_p = new TestArg{1, "hello"};
    mrpc::add_type(pointer, arg_p);
    EXPECT_EQ(pointer["type"], "pointer");
    EXPECT_TRUE(arg_json_eq(pointer["arg"], arg_rhs));
}

TEST(AddTypeTest, SharedPointer) {
    json shared_pointer;
    auto arg_sp = std::make_shared<TestArg>(TestArg{1, "hello"});
    mrpc::add_type(shared_pointer, arg_sp);
    EXPECT_EQ(shared_pointer["type"], "shared pointer");
    EXPECT_TRUE(arg_json_eq(shared_pointer["arg"], arg_rhs));
}

TEST(AddTypeTest, UniquePointer) {
    json unique_pointer;
    auto arg_up = std::make_unique<TestArg>(TestArg{1, "hello"});
    mrpc::add_type(unique_pointer, arg_up);
    EXPECT_EQ(unique_pointer["type"], "unique pointer");
    EXPECT_TRUE(arg_json_eq(unique_pointer["arg"], arg_rhs));
}

// todo: will be removed after implementing server
TEST(ArgListTest, ArgList) {
    auto func = mrpc::wrapper<int, int, TestArg>("func");
    TestArg obj{1, "hello"};
    std::string j = R"({"args":[{"arg":1,"type":"object"},{"arg":{"id":1,"name":"hello"},"type":"object"}],"func_name":"func","length":2})";
    EXPECT_EQ(func(1, std::move(obj)), j);
}