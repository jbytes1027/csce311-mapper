#include "Map.h"

#include <iostream>
#include <string>

Node::Node(int key, string value) {
    this->key = key;
    this->value = value;
    next = nullptr;
}

Map::Map(int numBuckets) {
    this->numBuckets = numBuckets;
    buckets = new Node*[numBuckets];

    for (int i = 0; i < numBuckets; i++) {
        buckets[i] = nullptr;
    }
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

bool Map::isKeyInBucket(int key, Node* head) {
    for (Node* node = head; node != nullptr; node = node->next) {
        if (node->key == key) {
            return true;
        }
    }
    return false;
}

bool Map::insert(int key, string value) {
    int bucket = hash(key);
    Node* bucketHead = buckets[bucket];

    // If key already exists, fail to insert
    if (isKeyInBucket(key, bucketHead)) return false;

    Node* newNode = new Node(key, value);
    newNode->next = bucketHead;
    // Make the new node the head of the bucket
    buckets[bucket] = newNode;

    return true;
}

string Map::lookup(int key) {
    int bucket = hash(key);

    for (Node* node = buckets[bucket]; node != nullptr; node = node->next) {
        if (node->key == key) {
            return node->value;
        }
    }

    return "";
}

bool Map::remove(int key) {
    int bucket = hash(key);

    // Start search from the head of the bucket
    Node* currNode = buckets[bucket];
    // Stores the previously visited node so that when the current node is removed,
    // the previous node's next pointer can be updated
    Node* prevNode = nullptr;
    // While not at the end of the bucket
    while (currNode != nullptr) {
        if (currNode->key == key) {
            // If at head
            if (prevNode == nullptr) {
                buckets[bucket] = currNode->next;
            } else {
                prevNode->next = currNode->next;
            }

            delete currNode;
            return true;
        }

        // Move forward
        prevNode = currNode;
        currNode = currNode->next;
    }

    return false;
}

// For debugging
void Map::printBuckets() {
    for (int i = 0; i < numBuckets; i++) {
        cout << i << ": ";
        printBucket(buckets[i]);
    }
}

// For debugging
void Map::printBucket(Node* head) {
    for (Node* node = head; node != nullptr; node = node->next) {
        cout << "(" << node->key << ", " << node->value << ") -> ";
    }
    cout << "\n";
}
