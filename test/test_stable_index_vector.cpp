#include <gtest/gtest.h>
#include "stable_index_vector.hpp"
#include <string>

namespace {

struct TestObject {
    int value;
    std::string name;
    TestObject(int v, std::string n) : value(v), name(n) {}
};

TEST(StableIndexVectorTest, PushAndGet) {
    siv::Vector<TestObject> vec;
    size_t id1 = vec.push_back({1, "one"});
    size_t id2 = vec.push_back({2, "two"});

    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[id1].value, 1);
    EXPECT_EQ(vec[id2].name, "two");
    
    EXPECT_NE(vec.get(id1), nullptr);
    EXPECT_EQ(vec.get(id1)->value, 1);
    EXPECT_EQ(vec.get(id2)->name, "two");
}

TEST(StableIndexVectorTest, EmplaceAndGet) {
    siv::Vector<TestObject> vec;
    size_t id = vec.emplace_back(3, "three");

    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[id].value, 3);
    EXPECT_EQ(vec[id].name, "three");
}

TEST(StableIndexVectorTest, EraseAndSwapBehavior) {
    siv::Vector<int> vec;
    size_t id0 = vec.push_back(0);
    size_t id1 = vec.push_back(1);
    size_t id2 = vec.push_back(2);

    // Erase middle element (id1)
    vec.erase(id1);

    EXPECT_EQ(vec.size(), 2);
    
    // Check remaining elements are still accessible by original IDs
    EXPECT_EQ(vec[id0], 0);
    EXPECT_EQ(vec[id2], 2);
    
    // Check erased element is gone
    EXPECT_EQ(vec.get(id1), nullptr);
}

TEST(StableIndexVectorTest, ReuseIDAfterErase) {
    siv::Vector<int> vec;
    size_t id0 = vec.push_back(0);
    size_t id1 = vec.push_back(1);
    
    vec.erase(id0);
    
    size_t new_id = vec.push_back(100);
    
    // The erased slot (which was originally id0) should be reused
    EXPECT_EQ(new_id, id0);
    EXPECT_EQ(vec[new_id], 100);
    EXPECT_EQ(vec[id1], 1);
}

TEST(StableIndexVectorTest, StabilityAfterMultipleErasures) {
    siv::Vector<int> vec;
    std::vector<size_t> ids;
    for(int i=0; i<10; ++i) {
        ids.push_back(vec.push_back(i));
    }
    
    // Erase a few
    vec.erase(ids[2]);
    vec.erase(ids[5]);
    vec.erase(ids[8]);
    
    EXPECT_EQ(vec.size(), 7);
    
    // Verify survivors
    EXPECT_EQ(vec[ids[0]], 0);
    EXPECT_EQ(vec[ids[1]], 1);
    EXPECT_EQ(vec[ids[3]], 3);
    EXPECT_EQ(vec[ids[4]], 4);
    EXPECT_EQ(vec[ids[6]], 6);
    EXPECT_EQ(vec[ids[7]], 7);
    EXPECT_EQ(vec[ids[9]], 9);
}

// ... (existing tests)

TEST(StableIndexVectorTest, ExercisesAllocationBranch) {
    siv::Vector<int> vec;
    // Pushing elements exercises the "else" branch (new allocation)
    size_t id1 = vec.push_back(1);
    size_t id2 = vec.push_back(2);
    
    EXPECT_EQ(vec[id1], 1);
    EXPECT_EQ(vec[id2], 2);
}

TEST(StableIndexVectorTest, ExercisesReuseBranchAndResize) {
    siv::Vector<int> vec;
    
    // Fill to create metadata
    for(int i = 0; i < 5; ++i) vec.push_back(i);
    
    // Erase something
    vec.erase(2); 
    
    // Now push back to exercise the "if" branch (reuse)
    size_t id = vec.push_back(100);
    EXPECT_EQ(id, 2);
    EXPECT_EQ(vec[id], 100);
}

} // namespace

