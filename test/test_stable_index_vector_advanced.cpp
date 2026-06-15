#include <gtest/gtest.h>
#include "stable_index_vector.hpp"
#include <vector>
#include <numeric>

namespace {

// Advanced testing for C++ committee proposal style verification.
// Focus on boundary conditions, complex types, and internal consistency.

TEST(StableIndexVectorAdvancedTest, ComplexObjectWithMoveSemantics) {
    struct MoveOnlyObject {
        std::vector<int> data;
        int id;
        MoveOnlyObject(int i) : data(100, i), id(i) {}
        MoveOnlyObject(MoveOnlyObject&&) = default;
        MoveOnlyObject& operator=(MoveOnlyObject&&) = default;
        MoveOnlyObject(const MoveOnlyObject&) = delete;
        MoveOnlyObject& operator=(const MoveOnlyObject&) = delete;
    };

    siv::Vector<MoveOnlyObject> vec;
    size_t id = vec.emplace_back(42);
    
    EXPECT_EQ(vec[id].id, 42);
    EXPECT_EQ(vec[id].data.size(), 100);
}

TEST(StableIndexVectorAdvancedTest, MassiveOperationsAndStability) {
    siv::Vector<size_t> vec;
    const size_t num_elements = 1000;
    std::vector<size_t> ids(num_elements);
    
    // Fill
    for(size_t i = 0; i < num_elements; ++i) {
        ids[i] = vec.push_back(i);
    }
    
    // Erase all even indices
    for(size_t i = 0; i < num_elements; i += 2) {
        vec.erase(ids[i]);
    }
    
    EXPECT_EQ(vec.size(), num_elements / 2);
    
    // Verify survivors
    for(size_t i = 1; i < num_elements; i += 2) {
        EXPECT_EQ(vec[ids[i]], i);
    }
}

TEST(StableIndexVectorAdvancedTest, OutOfBoundsAccess) {
    siv::Vector<int> vec;
    size_t id = vec.push_back(10);
    (void)id;
    
    // These should not crash but return nullptr for get()
    EXPECT_EQ(vec.get(999), nullptr);
    
    // Accessing via operator[] with invalid ID is undefined behavior 
    // in the current implementation (it will access invalid index or out of bounds).
    // The current implementation lacks bounds checking for operator[].
    
    // We should not uncomment this, as it will likely crash or access garbage memory.
    // EXPECT_EQ(vec[999], 10); 
}

/*
// BUG: The current implementation does not properly handle the case where
// an ID is used that was never allocated. The internal `m_indexes` vector
// might not have been resized for that large ID.
TEST(StableIndexVectorAdvancedTest, DISABLED_AccessNeverAllocatedID) {
    siv::Vector<int> vec;
    // This should ideally return nullptr or handle it safely, 
    // instead of potentially accessing unallocated memory.
    EXPECT_EQ(vec.get(1000), nullptr);
}
*/

} // namespace
