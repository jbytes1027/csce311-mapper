using namespace std;
#include <iostream>

class Node {
  public:
    Node(int);
    int value;
    Node* next;
};

class LinkedList {
  public:
    LinkedList();
    ~LinkedList();
    void insert(int);
    void remove(int);
    bool contains(int);
    void print();
    Node* head;
};

Node::Node(int value) {
    this->value = value;
    next = nullptr;
}

LinkedList::LinkedList() { head = nullptr; }

LinkedList::~LinkedList() {
    while (head != nullptr) {
        remove(head->value);
    }
}

void LinkedList::insert(int value) {
    Node* newNode = new Node(value);

    if (head == nullptr) {
        head = newNode;
    } else {
        newNode->next = head;
        head = newNode;
    }
}

void LinkedList::remove(int value) {
    Node* prevNode = nullptr;
    Node* currNode = head;

    while (currNode != nullptr) {
        if (currNode->value == value) {
            // update pointers around node
            if (prevNode != nullptr) {
                prevNode->next = currNode->next;
            } else {  // removing head
                head = currNode->next;
            }

            delete currNode;
            return;
        } else {
            prevNode = currNode;
            currNode = currNode->next;
        }
    }
}

void LinkedList::print() {
    for (Node* node = head; node != nullptr; node = node->next) {
        cout << node->value << " ";
    }
    cout << "\n";
}

bool LinkedList::contains(int value) {
    for (Node* node = head; node != nullptr; node = node->next) {
        if (node->value == value) {
            return true;
        }
    }

    return false;
}

int main() {
    LinkedList* ll = new LinkedList();
    ll->insert(1);
    ll->insert(5);
    ll->insert(9);
    ll->insert(2);
    ll->remove(2);
    ll->remove(1);
    ll->print();
    cout << ll->contains(1);
    delete ll;
}