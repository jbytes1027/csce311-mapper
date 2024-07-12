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
  protected:
    int numBuckets;

    Node** buckets;

    int hash(int);

    bool isKeyInBucket(int, Node*);

  public:
    Map(int numBuckets = 1000);

    ~Map();

    bool insert(int, string);

    bool remove(int);

    string lookup(int);

    void printBucket(Node*);

    void printBuckets();
};
