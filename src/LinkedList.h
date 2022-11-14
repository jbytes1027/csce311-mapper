#include "Entry.h"

class Node {
  public:
    Node(Entry*);
    ~Node();
    Entry* entry;
    Node* next;
};

class LinkedList {
  public:
    LinkedList();
    ~LinkedList();
    void insert(Entry*);
    void remove(Entry*);
    bool contains(Entry*);
    void print();
    Node* head;
};