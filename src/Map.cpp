using namespace std;

#include "Map.h"

#include <iostream>
#include <string>

Node::Node(int key, string value) {
    this->key = key;
    this->value = value;
    next = nullptr;
}

Map::Map(int threads, int numBuckets) {
    this->threads = threads;
    this->numBuckets = numBuckets;
    buckets = new Node*[numBuckets];
}

Map::~Map() {
    for (int i = 0; i < numBuckets; i++) {
        while (buckets[i] != nullptr) {
            remove(buckets[i]->key);
        }
    }

    delete buckets;
}

int Map::hash(int value) { return value % numBuckets; }

bool Map::keyInBucket(int key, Node* head) {
    for (Node* node = head; node != nullptr; node = node->next) {
        if (node->key == key) {
            return true;
        }
    }
    return false;
}

bool Map::insert(int key, string value) {
    int hashValue = hash(key);
    Node* head = buckets[hashValue];

    if (keyInBucket(key, head)) {
        return false;
    }

    Node* newNode = new Node(key, value);

    if (head == nullptr) {
        buckets[hashValue] = newNode;
        return true;
    }

    newNode->next = head;
    buckets[hashValue] = newNode;
    return true;
}

string Map::lookup(int key) {
    int hashValue = hash(key);

    Node* head = buckets[hashValue];

    for (Node* node = head; node != nullptr; node = node->next) {
        if (node->key == key) {
            return node->value;
        }
    }

    return "";
}

void Map::printBuckets() {
    for (int i = 0; i < numBuckets; i++) {
        cout << i << ": ";
        printBucket(buckets[i]);
    }
}

void Map::printBucket(Node* head) {
    for (Node* node = head; node != nullptr; node = node->next) {
        cout << "(" << node->key << ", " << node->value << ") -> ";
    }
    cout << "\n";
}

bool Map::remove(int key) {
    int hashValue = hash(key);
    Node* head = buckets[hashValue];

    Node* prevNode = nullptr;
    Node* currNode = head;
    while (currNode != nullptr) {
        if (currNode->key == key) {
            // update pointers around node
            if (prevNode != nullptr) {
                prevNode->next = currNode->next;
            } else {  // removing head
                buckets[hashValue] = currNode->next;
            }

            delete currNode;
            return true;
        } else {
            prevNode = currNode;
            currNode = currNode->next;
        }
    }

    return false;
}