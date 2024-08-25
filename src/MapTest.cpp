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

bool isOutputEqualWithoutThreadCount(stringstream* s1, stringstream* s2) {
    string line1;
    string line2;

    // Skip first line with thread info
    getline(*s1, line1);
    getline(*s2, line2);

    while (true) {
        if (s1->eof()) {
            if (s2->eof()) {
                // If both finish at the same time
                return true;
            } else {
                // If one finishes first
                return false;
            }
        }

        getline(*s1, line1);
        getline(*s2, line2);
        if (line1 != line2) return false;
    }
}

TEST_F(ThreadlessTest, Insert) {
    EXPECT_TRUE(map->insert(0, "a"));    // Test insertion into a empty bucket
    EXPECT_TRUE(map->insert(10, "b"));   // Test insertion into a non-empty bucket
    EXPECT_FALSE(map->insert(10, "c"));  // Test inserting a duplicate

    EXPECT_EQ(map->lookup(0), "a");
    EXPECT_EQ(map->lookup(10), "b");
}

TEST_F(ThreadlessTest, Remove) {
    EXPECT_TRUE(map->insert(0, "a"));    // Insertion into a empty bucket
    EXPECT_TRUE(map->insert(10, "b"));   // Insertion into a non-empty bucket
    EXPECT_TRUE(map->insert(100, "c"));  // Insertion into a non-empty bucket
    // bucket0: c -> b -> a

    EXPECT_FALSE(map->remove(1000));  // Test removing a non-existent key
    EXPECT_EQ(map->lookup(0), "a");
    EXPECT_EQ(map->lookup(10), "b");
    EXPECT_EQ(map->lookup(100), "c");

    EXPECT_TRUE(map->remove(10));  // Test removing a middle node
    EXPECT_EQ(map->lookup(0), "a");
    EXPECT_EQ(map->lookup(10), "");
    EXPECT_EQ(map->lookup(100), "c");

    EXPECT_TRUE(map->remove(100));  // Test removing a tail node
    EXPECT_EQ(map->lookup(0), "a");
    EXPECT_EQ(map->lookup(100), "");

    EXPECT_TRUE(map->remove(0));  // Test removing a head node
    EXPECT_EQ(map->lookup(0), "");
}

TEST_F(ThreadlessTest, Lookup) {
    EXPECT_TRUE(map->insert(0, "a"));    // Insertion into a empty bucket
    EXPECT_TRUE(map->insert(10, "b"));   // Insertion into a non-empty bucket
    EXPECT_TRUE(map->insert(100, "c"));  // Insertion into a non-empty bucket
    // bucket0: c -> b -> a

    EXPECT_EQ(map->lookup(0), "a");    // Lookup tail
    EXPECT_EQ(map->lookup(10), "b");   // Lookup middle
    EXPECT_EQ(map->lookup(100), "c");  // Lookup head

    EXPECT_EQ(map->lookup(0), "a");    // Lookup existing
    EXPECT_EQ(map->lookup(1000), "");  // Lookup non-existing in bucket with non-empty bucket

    EXPECT_EQ(map->lookup(1), "");  // Lookup non-existing in null bucket
}

TEST(ThreadedTest, StressTest) {
    stringstream treatInputStream;
    treatInputStream << "N 10\n";

    stringstream controlInputStream;
    controlInputStream << "N 1\n";

    int numOpp = 100000;
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

    EXPECT_TRUE(isOutputEqualWithoutThreadCount(&treatOutput, &controlOutput));
}

TEST(ThreadedTest, DISABLED_MapperRandomKeyScalingTimer) {
    int consumerThreads = 20;
    int numOpp = 4194304;  // 2^22

    std::mt19937 randGen;
    randGen.seed(time(nullptr));

    for (int threads = 1; threads <= consumerThreads; threads++) {
        stringstream inputStream;
        inputStream << "N " << threads << "\n";

        for (int i = 0; i < numOpp; i++) {
            int opp = randGen() % 3;
            int key = randGen() % 1000;

            if (opp == 0) {
                inputStream << "I " << key << " \"asdf\"\n";
            } else if (opp == 1) {
                inputStream << "L " << key << "\n";
            } else if (opp == 2) {
                inputStream << "D " << key << "\n";
            }
        }

        // https://stackoverflow.com/questions/22387586/measuring-execution-time-of-a-function-in-c
        chrono::system_clock::time_point begin;
        chrono::system_clock::time_point end;

        begin = chrono::high_resolution_clock::now();
        stringstream outputStream = executeStream(&inputStream);
        end = chrono::high_resolution_clock::now();
        int msExec = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
        cout << "Executed " << numOpp << " operations with " << threads << " consumer(s) in "
             << msExec << "ms\n";
    }
}

TEST(ThreadedTest, MapRandomKeyScalingTimer) {
TEST(ThreadedTest, DISABLED_MapRandomKeyScalingTimer) {
    int consumerThreads = 20;
    int mapOppDelayCycles = 40000;
    int numOpp = 10000;
    int buckets = 1000;

    std::mt19937 randGen;
    randGen.seed(time(nullptr));

    for (int threads = 1; threads <= consumerThreads; threads++) {
        stringstream inputStream;
        inputStream << "N " << threads << "\n";

        ConcurrentMap* map = new ConcurrentMap(buckets, mapOppDelayCycles);

        for (int i = 0; i < numOpp; i++) {
            int opp = randGen() % 3;
            int key = randGen() % 1000;

            if (opp == 0) {
                inputStream << "I " << key << " \"asdf\"\n";
            } else if (opp == 1) {
                inputStream << "L " << key << "\n";
            } else if (opp == 2) {
                inputStream << "D " << key << "\n";
            }
        }

        // https://stackoverflow.com/questions/22387586/measuring-execution-time-of-a-function-in-c
        chrono::system_clock::time_point begin;
        chrono::system_clock::time_point end;

        begin = chrono::high_resolution_clock::now();
        stringstream outputStream = executeStream(&inputStream, map);
        end = chrono::high_resolution_clock::now();
        int msExec = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
        cout << "Executed " << numOpp << " operations with " << threads << " consumer(s) in "
             << msExec << "ms\n";
    }
}

TEST(ThreadedTest, MapperRandomKeyScaling) {
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

    int numOpp = 4194304;  // 2^22
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

    // https://stackoverflow.com/questions/22387586/measuring-execution-time-of-a-function-in-c
    chrono::system_clock::time_point begin;
    chrono::system_clock::time_point end;

    begin = chrono::high_resolution_clock::now();
    stringstream outputStream1C = executeStream(&inputStream1C);
    end = chrono::high_resolution_clock::now();
    int msExec1C = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
    cout << "Executed " << numOpp << " operations with 1 consumer  in " << msExec1C << "ms\n";

    begin = chrono::high_resolution_clock::now();
    stringstream outputStream2C = executeStream(&inputStream2C);
    end = chrono::high_resolution_clock::now();
    int msExec2C = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
    cout << "Executed " << numOpp << " operations with 2 consumers in " << msExec2C << "ms\n";

    begin = chrono::high_resolution_clock::now();
    stringstream outputStream3C = executeStream(&inputStream3C);
    end = chrono::high_resolution_clock::now();
    int msExec3C = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
    cout << "Executed " << numOpp << " operations with 3 consumers in " << msExec3C << "ms\n";

    begin = chrono::high_resolution_clock::now();
    stringstream outputStream4C = executeStream(&inputStream4C);
    end = chrono::high_resolution_clock::now();
    int msExec4C = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
    cout << "Executed " << numOpp << " operations with 4 consumers in " << msExec4C << "ms\n";

    // Test correctness
    // Not comprehensive but good enough because stringstreams can't be copied
    EXPECT_TRUE(isOutputEqualWithoutThreadCount(&outputStream1C, &outputStream2C));
    EXPECT_TRUE(isOutputEqualWithoutThreadCount(&outputStream3C, &outputStream4C));

    // Test scaling
    // 2 consumers is faster than 1 consumer
    EXPECT_GT(msExec1C, msExec2C);
}

TEST(ThreadedTest, MapRandomKeyScaling) {
    // Add a delay to map operations to demonstrate map scaling
    // This makes the map operation runtime dwarf the runtime of the ordering logic
    int mapOppDelayCycles = 40000;
    ConcurrentMap* map1C = new ConcurrentMap(100, mapOppDelayCycles);
    ConcurrentMap* map2C = new ConcurrentMap(100, mapOppDelayCycles);
    ConcurrentMap* map3C = new ConcurrentMap(100, mapOppDelayCycles);
    ConcurrentMap* map4C = new ConcurrentMap(100, mapOppDelayCycles);

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

    int numOpp = 10000;
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

    // https://stackoverflow.com/questions/22387586/measuring-execution-time-of-a-function-in-c
    chrono::system_clock::time_point begin;
    chrono::system_clock::time_point end;

    begin = chrono::high_resolution_clock::now();
    stringstream outputStream1C = executeStream(&inputStream1C, map1C);
    end = chrono::high_resolution_clock::now();
    int msExec1C = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
    cout << "Executed " << numOpp << " operations with 1 consumer  in " << msExec1C << "ms\n";

    begin = chrono::high_resolution_clock::now();
    stringstream outputStream2C = executeStream(&inputStream2C, map2C);
    end = chrono::high_resolution_clock::now();
    int msExec2C = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
    cout << "Executed " << numOpp << " operations with 2 consumers in " << msExec2C << "ms\n";

    begin = chrono::high_resolution_clock::now();
    stringstream outputStream3C = executeStream(&inputStream3C, map3C);
    end = chrono::high_resolution_clock::now();
    int msExec3C = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
    cout << "Executed " << numOpp << " operations with 3 consumers in " << msExec3C << "ms\n";

    begin = chrono::high_resolution_clock::now();
    stringstream outputStream4C = executeStream(&inputStream4C, map4C);
    end = chrono::high_resolution_clock::now();
    int msExec4C = chrono::duration_cast<chrono::milliseconds>(end - begin).count();
    cout << "Executed " << numOpp << " operations with 4 consumers in " << msExec4C << "ms\n";

    // Test correctness
    // Not comprehensive but good enough because stringstreams can't be copied
    EXPECT_TRUE(isOutputEqualWithoutThreadCount(&outputStream1C, &outputStream2C));
    EXPECT_TRUE(isOutputEqualWithoutThreadCount(&outputStream3C, &outputStream4C));

    // Test scaling
    // 2 consumers is 1.5 times faster than 1 consumer
    EXPECT_GT((double)msExec1C / (double)msExec2C, 1.5);
}
