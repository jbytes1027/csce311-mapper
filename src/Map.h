using namespace std;

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
    int threads;
    int numBuckets;
    Node** buckets;
    int hash(int);
    bool keyInBucket(int, Node*);

  public:
    Map(int threads = 1, int numBuckets = 10);
    ~Map();
    bool insert(int, string);
    bool remove(int);
    string lookup(int);
    void printBucket(Node*);
    void printBuckets();
};
