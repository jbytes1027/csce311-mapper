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
    sem_t* sems;
    int hash(int);
    bool keyInBucket(int, Node*);
    void lock(int bucket);
    void unlock(int bucket);
    void signal(sem_t*);

  public:
    Map(int numBuckets = 100);
    ~Map();
    bool insert(int, string);
    string executeLineAndPost(string, sem_t*);
    bool remove(int);
    string lookup(int);
    void printBucket(Node*);
    void printBuckets();
};
