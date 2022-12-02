using namespace std;

#include <pthread.h>
#include <semaphore.h>

#include <string>

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
    // array of sems, one for each bucket
    sem_t* sems;
    int hash(int);
    bool keyInBucket(int, Node*);
    void lock(int bucket);
    void unlock(int bucket);
    // alias for sem_post with error checking
    void signal(sem_t*);
    // blocking sleep to pad opperation time to demonstrate scaling
    int numCyclesToSleepPerOpp;

  public:
    Map(int numBuckets = 100, int oppPaddingCycles = 0);
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
