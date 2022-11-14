using namespace std;
#include "LinkedList.h"

#include <iostream>
#include <string>

#include "Entry.h"

Node::Node(Entry* entry) {
    this->entry = entry;
    next = nullptr;
}

LinkedList::LinkedList() { head = nullptr; }

LinkedList::~LinkedList() {
    while (head != nullptr) {
        remove(head->entry);
    }
}

void LinkedList::insert(Entry* entry) {
    Node* newNode = new Node(entry);

    if (head == nullptr) {
        head = newNode;
    } else {
        newNode->next = head;
        head = newNode;
    }
}

void LinkedList::remove(Entry* entry) {
    Node* prevNode = nullptr;
    Node* currNode = head;

    while (currNode != nullptr) {
        if (currNode->entry == entry) {
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
        cout << "(" << node->entry->key << ", " << node->entry->value << ") ";
    }
    cout << "\n";
}

bool LinkedList::contains(Entry* entry) {
    for (Node* node = head; node != nullptr; node = node->next) {
        if (node->entry->key == entry->key) {
            return true;
        }
    }

    return false;
}

Node::~Node() { delete entry; }

int main() {
    LinkedList* ll = new LinkedList();
    Entry* e = new Entry(4, "string");
    ll->insert(e);
    ll->print();
    delete ll;
}