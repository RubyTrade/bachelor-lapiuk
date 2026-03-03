#include <gtest/gtest.h>

#include <array>
#include <string>

#include "core/utils/helper_utils.hpp"

enum class MyEnum { Invalid = 0, One = 1, Two = 2 };

static constexpr std::array<EnumStringPair<MyEnum>, 3> kPairs = {
    EnumStringPair<MyEnum>{MyEnum::Invalid, "invalid"},
    EnumStringPair<MyEnum>{MyEnum::One, "one"},
    EnumStringPair<MyEnum>{MyEnum::Two, "two"},
};

TEST(HelperUtils, StrToTypeReturnsMatchingOrDefault) {
  EXPECT_EQ(str_to_type(kPairs, std::string("one")), MyEnum::One);
  EXPECT_EQ(str_to_type(kPairs, std::string("two")), MyEnum::Two);

  // unknown -> default (first entry)
  EXPECT_EQ(str_to_type(kPairs, std::string("nope")), MyEnum::Invalid);
}

TEST(HelperUtils, TypeToStrReturnsMatchingOrInvalidFallback) {
  EXPECT_EQ(type_to_str(kPairs, MyEnum::One), "one");
  EXPECT_EQ(type_to_str(kPairs, MyEnum::Two), "two");

  // not in array -> "invalid"
  EXPECT_EQ(type_to_str(kPairs, static_cast<MyEnum>(999)), "invalid");
}

TEST(HelperUtils, CaseInsensitiveMatching) {
  // Assuming str_to_type is case-sensitive by default
  EXPECT_EQ(str_to_type(kPairs, std::string("one")), MyEnum::One);
  EXPECT_EQ(str_to_type(kPairs, std::string("ONE")), MyEnum::Invalid); // Different case
}

TEST(HelperUtils, EmptyStringReturnsDefault) {
  EXPECT_EQ(str_to_type(kPairs, std::string("")), MyEnum::Invalid);
}

TEST(HelperUtils, AllEnumValuesCanBeConverted) {
  std::vector<MyEnum> values = {MyEnum::Invalid, MyEnum::One, MyEnum::Two};
  
  for (const auto& val : values) {
    std::string str = type_to_str(kPairs, val);
    EXPECT_FALSE(str.empty());
    // Convert back
    MyEnum converted = str_to_type(kPairs, str);
    EXPECT_EQ(converted, val);
  }
}
