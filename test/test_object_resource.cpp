#include "../src/object_resource.hpp"
#include <gtest/gtest.h>

template <typename T, typename ReturnType = decltype(T::id)>
struct GetMockIdField {
  static ReturnType get(const T *t) { return t->id; };
};

struct MockObject {
  std::string id;
  int value;

  MockObject(std::string id, int value) : id(id), value(value) {}
};

class ObjectResourceTest : public ::testing::Test {
protected:
  ObjectResource<MockObject, GetMockIdField<MockObject>> resource;
};

TEST_F(ObjectResourceTest, CreateAndFind) {
  auto obj = resource.create("ID1", 100);
  ASSERT_NE(obj, nullptr);
  EXPECT_EQ(obj->id, "ID1");
  EXPECT_EQ(obj->value, 100);

  auto found = resource.find("ID1");
  EXPECT_EQ(found, obj);

  EXPECT_TRUE(resource.contains("ID1"));
}

TEST_F(ObjectResourceTest, FindNonExistent) {
  EXPECT_FALSE(resource.contains("NOT_EXISTING"));
  EXPECT_EQ(resource.find("NOT_EXISTING"), nullptr);
}

TEST_F(ObjectResourceTest, Erase) {
  resource.create("ID1", 100);
  resource.erase("ID1");

  EXPECT_FALSE(resource.contains("ID1"));
  EXPECT_EQ(resource.find("ID1"), nullptr);
}

struct MockIntObject {
  int id;
  int value;
  MockIntObject(int id, int value) : id(id), value(value) {}
};

TEST_F(ObjectResourceTest, DuplicateIdBehavior) {
  // Note: The current ObjectResource implementation doesn't check for
  // duplicate IDs in create(). This test documents that behavior.
  auto obj1 = resource.create("ID1", 100);
  auto obj2 = resource.create("ID1", 200);

  ASSERT_NE(obj1, nullptr);
  ASSERT_NE(obj2, nullptr);

  // Find() returns the pointer to the object in the set.
  // If multiple are inserted with the same ID, it's undefined which one is
  // returned by the set, but it will be one of them.
  auto found = resource.find("ID1");
  EXPECT_TRUE(found == obj1 || found == obj2);
}

TEST(ObjectResourceIntTest, KeyEqualBranchCoverage) {
  ObjectResource<MockIntObject, GetMockIdField<MockIntObject>> intResource;
  intResource.create(1, 100);

  // This triggers the KeyEqual operators:
  // bool operator()(const T *lhs, const ID &rhs)
  // bool operator()(const ID &lhs, const T *rhs)
  EXPECT_TRUE(intResource.contains(1));
  EXPECT_NE(intResource.find(1), nullptr);
}

TEST_F(ObjectResourceTest, HeterogeneousLookup) {
  resource.create("ID1", 100);

  // std::string
  EXPECT_TRUE(resource.contains(std::string("ID1")));
  // std::string_view
  EXPECT_TRUE(resource.contains(std::string_view("ID1")));
  // const char*
  EXPECT_TRUE(resource.contains("ID1"));

  EXPECT_NE(resource.find(std::string("ID1")), nullptr);
  EXPECT_NE(resource.find(std::string_view("ID1")), nullptr);
  EXPECT_NE(resource.find("ID1"), nullptr);
}
