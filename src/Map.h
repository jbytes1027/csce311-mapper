#include <pthread.h>
#include <semaphore.h>

#include <string>

using namespace std;

class Node {
  public:
    Node(int, string);
    int key;
    string value;
    Node* next;
};

class Map {
  private:
    int numBuckets;

    Node** buckets;

    // Array of sems, one for each bucket
    sem_t* sems;

    int hash(int);

    bool isKeyInBucket(int, Node*);

    void lock(int bucket);

    void unlock(int bucket);

    void signal(sem_t*);

    int numCyclesToSleepPerOpp;

  public:
    Map(int numBuckets = 1000, int oppPaddingCycles = 0);

    ~Map();

    bool insert(int, string);

    bool concurrentInsertAndPost(int, string, sem_t*);

    bool remove(int);

    bool concurrentRemoveAndPost(int, sem_t*);

    string lookup(int);

    string concurrentLookupAndPost(int, sem_t*);

    void printBucket(Node*);

    void printBuckets();
};
