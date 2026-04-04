#include <gtest/gtest.h>

#include "utils/PersistentMap.h"

#include <set>
#include <string>

TEST(PersistentMap, EmptyMap) {
    PersistentMap<int, std::string> m;
    EXPECT_EQ(m.size(), 0u);
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.find(42), nullptr);
}

TEST(PersistentMap, InsertAndFind) {
    PersistentMap<int, std::string> m;
    auto m2 = m.set(1, "one");
    auto m3 = m2.set(2, "two");
    auto m4 = m3.set(3, "three");

    EXPECT_EQ(m.size(), 0u);
    EXPECT_EQ(m2.size(), 1u);
    EXPECT_EQ(m3.size(), 2u);
    EXPECT_EQ(m4.size(), 3u);

    EXPECT_EQ(m.find(1), nullptr);
    EXPECT_NE(m2.find(1), nullptr);
    EXPECT_EQ(*m2.find(1), "one");
    EXPECT_EQ(*m4.find(1), "one");
    EXPECT_EQ(*m4.find(2), "two");
    EXPECT_EQ(*m4.find(3), "three");
}

TEST(PersistentMap, UpdateExistingKey) {
    auto m = PersistentMap<int, int>().set(1, 100);
    auto m2 = m.set(1, 200);

    EXPECT_EQ(m.size(), 1u);
    EXPECT_EQ(m2.size(), 1u);
    EXPECT_EQ(*m.find(1), 100);
    EXPECT_EQ(*m2.find(1), 200);
}

TEST(PersistentMap, Erase) {
    auto m = PersistentMap<int, int>().set(1, 10).set(2, 20).set(3, 30);
    EXPECT_EQ(m.size(), 3u);

    auto m2 = m.erase(2);
    EXPECT_EQ(m2.size(), 2u);
    EXPECT_NE(m2.find(1), nullptr);
    EXPECT_EQ(m2.find(2), nullptr);
    EXPECT_NE(m2.find(3), nullptr);

    // Original is unchanged
    EXPECT_EQ(m.size(), 3u);
    EXPECT_NE(m.find(2), nullptr);
}

TEST(PersistentMap, EraseNonexistent) {
    auto m = PersistentMap<int, int>().set(1, 10);
    auto m2 = m.erase(999);
    EXPECT_EQ(m2.size(), 1u);
}

TEST(PersistentMap, EraseToEmpty) {
    auto m = PersistentMap<int, int>().set(1, 10);
    auto m2 = m.erase(1);
    EXPECT_EQ(m2.size(), 0u);
    EXPECT_TRUE(m2.empty());
    EXPECT_EQ(m2.find(1), nullptr);
}

TEST(PersistentMap, StructuralSharing) {
    // Insert 1000 items, then create a new version with one more.
    // The old and new versions should share most of the structure.
    PersistentMap<int, int> m;
    for (int i = 0; i < 1000; ++i)
        m = m.set(i, i * 10);

    EXPECT_EQ(m.size(), 1000u);
    auto m2 = m.set(1000, 10000);
    EXPECT_EQ(m2.size(), 1001u);

    // Both versions should have all their data
    for (int i = 0; i < 1000; ++i) {
        EXPECT_EQ(*m.find(i), i * 10);
        EXPECT_EQ(*m2.find(i), i * 10);
    }
    EXPECT_EQ(m.find(1000), nullptr);
    EXPECT_EQ(*m2.find(1000), 10000);
}

TEST(PersistentMap, Iteration) {
    auto m = PersistentMap<int, int>().set(3, 30).set(1, 10).set(2, 20);

    std::set<int> keys;
    int sum = 0;
    for (auto it = m.begin(); it != m.end(); ++it) {
        auto e = *it;
        keys.insert(e.key);
        sum += e.value;
    }
    EXPECT_EQ(keys.size(), 3u);
    EXPECT_TRUE(keys.count(1));
    EXPECT_TRUE(keys.count(2));
    EXPECT_TRUE(keys.count(3));
    EXPECT_EQ(sum, 60);
}

TEST(PersistentMap, ForEach) {
    auto m = PersistentMap<int, int>().set(1, 10).set(2, 20).set(3, 30);
    int count = 0;
    int sum = 0;
    m.for_each([&](const int& k, const int& v) {
        ++count;
        sum += v;
    });
    EXPECT_EQ(count, 3);
    EXPECT_EQ(sum, 60);
}

TEST(PersistentMap, StringKeys) {
    PersistentMap<std::string, int> m;
    m = m.set("hello", 1).set("world", 2).set("test", 3);
    EXPECT_EQ(m.size(), 3u);
    EXPECT_EQ(*m.find("hello"), 1);
    EXPECT_EQ(*m.find("world"), 2);
    EXPECT_EQ(m.find("missing"), nullptr);
}

TEST(PersistentMap, LargeScale) {
    PersistentMap<uint64_t, uint64_t> m;
    constexpr int N = 10000;

    for (uint64_t i = 0; i < N; ++i)
        m = m.set(i, i * 100);

    EXPECT_EQ(m.size(), static_cast<size_t>(N));

    // Verify all entries
    for (uint64_t i = 0; i < N; ++i) {
        auto* v = m.find(i);
        ASSERT_NE(v, nullptr) << "key " << i << " not found";
        EXPECT_EQ(*v, i * 100);
    }

    // Erase half
    auto m2 = m;
    for (uint64_t i = 0; i < N; i += 2)
        m2 = m2.erase(i);

    EXPECT_EQ(m2.size(), static_cast<size_t>(N / 2));
    EXPECT_EQ(m.size(), static_cast<size_t>(N)); // original unchanged

    // Verify remaining
    for (uint64_t i = 0; i < N; ++i) {
        if (i % 2 == 0)
            EXPECT_EQ(m2.find(i), nullptr);
        else
            EXPECT_EQ(*m2.find(i), i * 100);
    }
}

TEST(PersistentMap, IterationCount) {
    PersistentMap<int, int> m;
    for (int i = 0; i < 100; ++i)
        m = m.set(i, i);

    int count = 0;
    for (auto it = m.begin(); it != m.end(); ++it)
        ++count;
    EXPECT_EQ(count, 100);
}

TEST(PersistentMap, SharedPtrValues) {
    // Mimics the session map use case
    PersistentMap<uint64_t, std::shared_ptr<std::string>> m;

    auto s1 = std::make_shared<std::string>("session1");
    auto s2 = std::make_shared<std::string>("session2");

    auto m2 = m.set(1, s1).set(2, s2);
    EXPECT_EQ(**m2.find(1), "session1");
    EXPECT_EQ(**m2.find(2), "session2");

    // Values are shared (not copied)
    EXPECT_EQ(m2.find(1)->get(), s1.get());

    auto m3 = m2.erase(1);
    EXPECT_EQ(m3.find(1), nullptr);
    EXPECT_EQ(**m3.find(2), "session2");

    // s1 still alive (m2 holds it)
    EXPECT_EQ(*s1, "session1");
}
