using namespace std;
#include <chrono>
#include <random>
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

    int numOpp = 10000;
    for (int i = 0; i < numOpp / 3; i++) {
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

TEST(ThreadedTest, PerformanceScaling) {
    stringstream inputStream1C;
    inputStream1C << "N 1\n";

    stringstream inputStream2C;
    inputStream2C << "N 2\n";

    stringstream inputStream3C;
    inputStream3C << "N 3\n";

    stringstream inputStream4C;
    inputStream4C << "N 4\n";

    std::mt19937 randGen;
    randGen.seed(time(nullptr));

    int numOpp = 100000;
    for (int i = 0; i < numOpp; i++) {
        int opp = randGen() % 3;
        int key = randGen() % 1000;

        if (opp == 0) {
            inputStream1C << "I " << key << " \"asdf\"\n";
            inputStream2C << "I " << key << " \"asdf\"\n";
            inputStream3C << "I " << key << " \"asdf\"\n";
            inputStream4C << "I " << key << " \"asdf\"\n";
        } else if (opp == 1) {
            inputStream1C << "L " << key << "\n";
            inputStream2C << "L " << key << "\n";
            inputStream3C << "L " << key << "\n";
            inputStream4C << "L " << key << "\n";
        } else if (opp == 2) {
            inputStream1C << "D " << key << "\n";
            inputStream2C << "D " << key << "\n";
            inputStream3C << "D " << key << "\n";
            inputStream4C << "D " << key << "\n";
        }
    }

    chrono::system_clock::time_point begin;
    chrono::system_clock::time_point end;

    begin = chrono::high_resolution_clock::now();
    executeStream(&inputStream1C);
    end = chrono::high_resolution_clock::now();
    cout << "Executed " << numOpp << " operations with 1 consumer  in "
         << chrono::duration_cast<chrono::milliseconds>(end - begin).count()
         << "ms\n";

    begin = chrono::high_resolution_clock::now();
    executeStream(&inputStream2C);
    end = chrono::high_resolution_clock::now();
    cout << "Executed " << numOpp << " operations with 2 consumers in "
         << chrono::duration_cast<chrono::milliseconds>(end - begin).count()
         << "ms\n";

    begin = chrono::high_resolution_clock::now();
    executeStream(&inputStream3C);
    end = chrono::high_resolution_clock::now();
    cout << "Executed " << numOpp << " operations with 3 consumers in "
         << chrono::duration_cast<chrono::milliseconds>(end - begin).count()
         << "ms\n";

    begin = chrono::high_resolution_clock::now();
    executeStream(&inputStream4C);
    end = chrono::high_resolution_clock::now();
    cout << "Executed " << numOpp << " operations with 4 consumers in "
         << chrono::duration_cast<chrono::milliseconds>(end - begin).count()
         << "ms\n";
}