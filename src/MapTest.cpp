using namespace std;
#include <string>

#include "Mapper.h"
#include "gtest/gtest.h"

class ThreadlessTest : public ::testing ::Test {
  protected:
    Map* map;
    void SetUp() override { map = new Map(10); };
    void TearDown() override { delete map; }
};

bool outputEqualIgnoreThreadCount(stringstream* s1, stringstream* s2) {
    string line1;
    string line2;

    // skip first line with thread info
    getline(*s1, line1);
    getline(*s2, line2);

    while (true) {
        if (s1->eof())
            if (s2->eof())
                return true;
            else
                return false;

        getline(*s1, line1);
        getline(*s2, line2);
        if (line1 != line2) return false;
    }
}

TEST_F(ThreadlessTest, Insert) {
    EXPECT_TRUE(map->insert(0, "a"));    // test ins to empty bucket
    EXPECT_TRUE(map->insert(10, "b"));   // test ins to non-empty bucket
    EXPECT_FALSE(map->insert(10, "c"));  // test ins duplicate

    EXPECT_EQ(map->lookup(0), "a");
    EXPECT_EQ(map->lookup(10), "b");
}

TEST_F(ThreadlessTest, Remove) {
    EXPECT_TRUE(map->insert(0, "a"));    // ins to empty bucket
    EXPECT_TRUE(map->insert(10, "b"));   // ins to non-empty bucket
    EXPECT_TRUE(map->insert(100, "c"));  // ins to non-empty bucket
    // bucket0: c -> b -> a

    EXPECT_FALSE(map->remove(1000));  // test remove non-existant
    EXPECT_EQ(map->lookup(0), "a");
    EXPECT_EQ(map->lookup(10), "b");
    EXPECT_EQ(map->lookup(100), "c");

    EXPECT_TRUE(map->remove(10));  // test remove middle
    EXPECT_EQ(map->lookup(0), "a");
    EXPECT_EQ(map->lookup(10), "");
    EXPECT_EQ(map->lookup(100), "c");

    EXPECT_TRUE(map->remove(100));  // test remove tail
    EXPECT_EQ(map->lookup(0), "a");
    EXPECT_EQ(map->lookup(100), "");

    EXPECT_TRUE(map->remove(0));  // test remove head
    EXPECT_EQ(map->lookup(0), "");
}

TEST_F(ThreadlessTest, Lookup) {
    EXPECT_TRUE(map->insert(0, "a"));    // ins to empty bucket
    EXPECT_TRUE(map->insert(10, "b"));   // ins to non-empty bucket
    EXPECT_TRUE(map->insert(100, "c"));  // ins to non-empty bucket
    // bucket0: c -> b -> a

    EXPECT_EQ(map->lookup(0), "a");    // lookup tail
    EXPECT_EQ(map->lookup(10), "b");   // lookup middle
    EXPECT_EQ(map->lookup(100), "c");  // lookup head

    EXPECT_EQ(map->lookup(0), "a");  // lookup existing
    EXPECT_EQ(map->lookup(1000),
              "");  // lookup non-existing in bucket with non-empty bucket

    EXPECT_EQ(map->lookup(1), "");  // lookup non-existing in null bucket
}

TEST(ThreadedTest, StressTest) {
    stringstream treatInputStream;
    treatInputStream << "N 10\n";

    stringstream controlInputStream;
    controlInputStream << "N 1\n";

    int numOpp = 10000 / 3;
    for (int i = 0; i < numOpp; i++) {
        treatInputStream << "I 1 \"asdf\"\n";
        treatInputStream << "L 1\n";
        treatInputStream << "D 1\n";

        controlInputStream << "I 1 \"asdf\"\n";
        controlInputStream << "L 1\n";
        controlInputStream << "D 1\n";
    }

    stringstream treatOutput = executeStream(&treatInputStream);
    stringstream controlOutput = executeStream(&controlInputStream);

    EXPECT_TRUE(outputEqualIgnoreThreadCount(&treatOutput, &controlOutput));
}