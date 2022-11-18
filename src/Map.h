using namespace std;

#include <pthread.h>

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
    pthread_mutex_t* locks;
    int hash(int);
    bool keyInBucket(int, Node*);
    void lock(int bucket);
    void unlock(int bucket);

  public:
    Map(int numBuckets = 10);
    ~Map();
    bool insert(int, string);
    bool remove(int);
    string lookup(int);
    void printBucket(Node*);
    void printBuckets();
};
