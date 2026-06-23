#include "object_resource.hpp"
#include "order_id.hpp"
#include <gtest/gtest.h>

TEST(FixedSizeOrderIDTest, BasicFunctionality) {
  FixedSizeOrderID id1("A001");
  FixedSizeOrderID id2("A001");
  FixedSizeOrderID id3("B002");

  EXPECT_EQ(id1, id2);
  EXPECT_NE(id1, id3);
  EXPECT_EQ(static_cast<std::string_view>(id1), "A001");
}

TEST(FixedSizeOrderIDTest, HeterogeneousLookupCompatibility) {
  static_assert(StringViewNormalizable<FixedSizeOrderID>,
                "FixedSizeOrderID should satisfy StringViewNormalizable");

  FixedSizeOrderID id("A001");
  std::string_view sv = "A001";

  EXPECT_EQ(id, sv);
  EXPECT_EQ(sv, id);

  EXPECT_EQ(std::hash<FixedSizeOrderID>{}(id),
            std::hash<std::string_view>{}(sv));
}
