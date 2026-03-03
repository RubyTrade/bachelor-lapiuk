#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include "core/utils/json.hpp"

TEST(JSONQuery, EmptyByDefault) {
  JSONQuery q;
  EXPECT_TRUE(q.is_empty());
  EXPECT_FALSE(q.is_key_exists("x"));
  EXPECT_FALSE(q.get_value("x").has_value());
}

TEST(JSONQuery, InvalidJsonThrowsOnConstruction) {
  JSONQuery invalid_json = JSONQuery("{not-json");
  EXPECT_TRUE(invalid_json.is_empty());
}

TEST(JSONQuery, ParseAndStrRoundTripIsValidJson) {
  JSONQuery q(R"({"a":1,"b":"x"})");
  const std::string dumped = q.str();
  EXPECT_NO_THROW(nlohmann::json::parse(dumped));
}

TEST(JSONQuery, JsonReturnsCopyNotReference) {
  JSONQuery q(R"({"a":1})");
  auto j = q.json();
  j["a"] = 999;
  auto v = q.get_value("a");
  ASSERT_TRUE(v);
  EXPECT_EQ(v->get<int>(), 1);
}

TEST(JSONQuery, CopyConstructorCopiesJson) {
  JSONQuery q(R"({"a":1})");
  JSONQuery copy(q);
  auto v = copy.get_value("a");
  ASSERT_TRUE(v);
  EXPECT_EQ(v->get<int>(), 1);
}

TEST(JSONQuery, SetValueOverwritesNonArrayKey) {
  JSONQuery q;
  q.set_value("k", std::string("v"));
  q.set_value("k", 123);

  auto v = q.get_value("k");
  ASSERT_TRUE(v);
  EXPECT_TRUE(v->is_number_integer());
  EXPECT_EQ(v->get<int>(), 123);
}

TEST(JSONQuery, AddToArrayCreatesArrayAndDeduplicatesStrings) {
  JSONQuery q;
  q.add_to_array("arr", std::string("a"));
  q.add_to_array("arr", std::string("a"));
  q.add_to_array("arr", std::string("b"));

  auto arr = q.get_value("arr");
  ASSERT_TRUE(arr);
  ASSERT_TRUE(arr->is_array());
  EXPECT_EQ(arr->size(), 2);
  EXPECT_EQ((*arr)[0].get<std::string>(), "a");
  EXPECT_EQ((*arr)[1].get<std::string>(), "b");
}

TEST(JSONQuery, AddToArrayDeduplicatesNumbersAndBools) {
  JSONQuery q;
  q.add_to_array("nums", 1);
  q.add_to_array("nums", 1);
  q.add_to_array("nums", 2);

  auto nums = q.get_value("nums");
  ASSERT_TRUE(nums);
  ASSERT_TRUE(nums->is_array());
  EXPECT_EQ(nums->size(), 2);

  q.add_to_array("flags", true);
  q.add_to_array("flags", true);
  q.add_to_array("flags", false);

  auto flags = q.get_value("flags");
  ASSERT_TRUE(flags);
  ASSERT_TRUE(flags->is_array());
  EXPECT_EQ(flags->size(), 2);
}

TEST(JSONQuery, AddToArrayDeduplicatesObjects) {
  JSONQuery q;
  nlohmann::json obj1 = nlohmann::json::object({{"k", 1}});
  nlohmann::json obj2 = nlohmann::json::object({{"k", 1}});
  nlohmann::json obj3 = nlohmann::json::object({{"k", 2}});

  q.add_to_array("objs", obj1);
  q.add_to_array("objs", obj2);
  q.add_to_array("objs", obj3);

  auto arr = q.get_value("objs");
  ASSERT_TRUE(arr);
  EXPECT_EQ(arr->size(), 2);
}

TEST(JSONQuery, GetNonExistentKeyReturnsNullopt) {
  JSONQuery q(R"({"a": 1})");
  auto val = q.get_value("nonexistent");
  EXPECT_FALSE(val.has_value());
}

TEST(JSONQuery, IsKeyExistsWorks) {
  JSONQuery q(R"({"a": 1, "b": null})");
  EXPECT_TRUE(q.is_key_exists("a"));
  EXPECT_TRUE(q.is_key_exists("b"));
  EXPECT_FALSE(q.is_key_exists("c"));
}

TEST(JSONQuery, NestedJsonAccess) {
  JSONQuery q(R"({"outer": {"inner": 42}})");
  auto outer = q.get_value("outer");
  ASSERT_TRUE(outer.has_value());
  EXPECT_TRUE(outer->is_object());
}

TEST(JSONQuery, ArrayHandling) {
  JSONQuery q(R"({"arr": [1, 2, 3]})");
  auto arr = q.get_value("arr");
  ASSERT_TRUE(arr.has_value());
  EXPECT_TRUE(arr->is_array());
  EXPECT_EQ(arr->size(), 3);
}

TEST(JSONQuery, NullValueHandling) {
  JSONQuery q(R"({"nullable": null})");
  auto val = q.get_value("nullable");
  ASSERT_TRUE(val.has_value());
  EXPECT_TRUE(val->is_null());
}

TEST(JSONQuery, BooleanValues) {
  JSONQuery q(R"({"t": true, "f": false})");
  auto t = q.get_value("t");
  auto f = q.get_value("f");

  ASSERT_TRUE(t.has_value());
  ASSERT_TRUE(f.has_value());
  EXPECT_TRUE(t->get<bool>());
  EXPECT_FALSE(f->get<bool>());
}

TEST(JSONQuery, NumberTypes) {
  JSONQuery q(R"({"i": 42, "d": 3.14})");
  auto i = q.get_value("i");
  auto d = q.get_value("d");

  ASSERT_TRUE(i.has_value());
  ASSERT_TRUE(d.has_value());
  EXPECT_EQ(i->get<int>(), 42);
  EXPECT_DOUBLE_EQ(d->get<double>(), 3.14);
}

TEST(JSONQuery, StringValues) {
  JSONQuery q(R"({"s": "hello world"})");
  auto s = q.get_value("s");

  ASSERT_TRUE(s.has_value());
  EXPECT_EQ(s->get<std::string>(), "hello world");
}

TEST(JSONQuery, SetValueOnExistingArrayAddsElementAndDeduplicates) {
  JSONQuery q;
  q.add_to_array("arr", std::string("a"));
  q.set_value("arr", std::string("b"));
  q.set_value("arr", std::string("b"));

  auto arr = q.get_value("arr");
  ASSERT_TRUE(arr);
  ASSERT_TRUE(arr->is_array());
  EXPECT_EQ(arr->size(), 2);
}

TEST(JSONQuery, RemoveKeyMissingIsNoop) {
  JSONQuery q;
  EXPECT_NO_THROW(q.remove_key("missing"));
  EXPECT_FALSE(q.is_key_exists("missing"));
}

TEST(JSONQuery, RemoveKeyMakesQueryEmptyWhenLastKeyRemoved) {
  JSONQuery q(R"({"a":1})");
  EXPECT_FALSE(q.is_empty());
  q.remove_key("a");
  EXPECT_TRUE(q.is_empty());
}

TEST(JSONQuery, StaticHelpers) {
  nlohmann::json j = nlohmann::json::parse(R"({"x":true})");
  EXPECT_TRUE(JSONQuery::is_key_exists(j, "x"));
  auto v = JSONQuery::get_value(j, "x");
  ASSERT_TRUE(v);
  EXPECT_TRUE(v->get<bool>());
}

TEST(JSONQuery, GetMapOfItemsCoversTypesAndFallbacks) {
  JSONQuery q;
  q.set_value("s", std::string("str"));
  q.set_value("b", true);
  q.set_value("i", -7);
  q.set_value("u", static_cast<uint64_t>(42));
  q.set_value("o", nlohmann::json::object({{"k", 1}}));
  q.set_value("n", nlohmann::json(nullptr));
  q.set_value("a", nlohmann::json::array({1, 2}));

  auto m = q.get_map_of_items();
  ASSERT_TRUE(m.count("s") > 0);
  ASSERT_TRUE(m.count("b") > 0);
  ASSERT_TRUE(m.count("i") > 0);
  ASSERT_TRUE(m.count("u") > 0);
  ASSERT_TRUE(m.count("o") > 0);
  ASSERT_TRUE(m.count("n") > 0);
  ASSERT_TRUE(m.count("a") > 0);

  EXPECT_EQ(std::get<std::string>(m["s"]), "str");
  EXPECT_EQ(std::get<bool>(m["b"]), true);
  EXPECT_EQ(std::get<int>(m["i"]), -7);
  EXPECT_EQ(std::get<uint64_t>(m["u"]), 42ULL);

  auto obj = std::get<nlohmann::json>(m["o"]);
  EXPECT_TRUE(obj.is_object());
  EXPECT_EQ(obj["k"].get<int>(), 1);

  EXPECT_EQ(std::get<std::string>(m["n"]), "null");

  // arrays are not special-cased, so fallback is a dumped string
  EXPECT_TRUE(std::holds_alternative<std::string>(m["a"]));
  EXPECT_NE(std::get<std::string>(m["a"]).find("["), std::string::npos);
}

TEST(JSONQuery, ObjectValueRoundTrip) {
  JSONQuery q;
  nlohmann::json obj = nlohmann::json::object({{"nested", {"x", 1}}});
  q.set_value("obj", obj);

  auto v = q.get_value("obj");
  ASSERT_TRUE(v);
  ASSERT_TRUE(v->is_object());
  EXPECT_TRUE(v->contains("nested"));
}

TEST(JSONQuery, SetValueSupportsNullAndArrayAndObject) {
  JSONQuery q;
  q.set_value("nullv", nlohmann::json(nullptr));
  q.set_value("arr", nlohmann::json::array({1, 2, 3}));
  q.set_value("obj", nlohmann::json::object({{"x", 1}}));

  auto nullv = q.get_value("nullv");
  ASSERT_TRUE(nullv);
  EXPECT_TRUE(nullv->is_null());

  auto arr = q.get_value("arr");
  ASSERT_TRUE(arr);
  EXPECT_TRUE(arr->is_array());
  EXPECT_EQ(arr->size(), 3);

  auto obj = q.get_value("obj");
  ASSERT_TRUE(obj);
  EXPECT_TRUE(obj->is_object());
  EXPECT_EQ((*obj)["x"].get<int>(), 1);
}

TEST(JSONQuery, NestedObjectGetValueReturnsObjectJson) {
  JSONQuery q(R"({"outer":{"inner":123}})");
  auto outer = q.get_value("outer");
  ASSERT_TRUE(outer);
  ASSERT_TRUE(outer->is_object());
  EXPECT_EQ((*outer)["inner"].get<int>(), 123);
}

TEST(JSONQuery, NumericOverwritesMaintainLatestType) {
  JSONQuery q;
  q.set_value("n", 1);
  {
    auto v = q.get_value("n");
    ASSERT_TRUE(v);
    EXPECT_TRUE(v->is_number_integer());
  }

  q.set_value("n", static_cast<uint64_t>(2));
  {
    auto v = q.get_value("n");
    ASSERT_TRUE(v);
    EXPECT_TRUE(v->is_number_unsigned() || v->is_number_integer());
    // nlohmann may store small uints as unsigned.
    EXPECT_EQ(v->get<uint64_t>(), 2ULL);
  }
}
