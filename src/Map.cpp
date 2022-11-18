using namespace std;

#include "Map.h"

#include <pthread.h>

#include <iostream>
#include <string>

void Map::lock(int bucket) {
    int res = -1;
    while (res != 0) {
        res = pthread_mutex_lock(&locks[bucket]);
    }
}

void Map::unlock(int bucket) {
    int res = -1;
    while (res != 0) {
        res = pthread_mutex_unlock(&locks[bucket]);
    }
}

Node::Node(int key, string value) {
    this->key = key;
    this->value = value;
    next = nullptr;
}

Map::Map(int threads, int numBuckets) {
    this->threads = threads;
    this->numBuckets = numBuckets;
    buckets = new Node*[numBuckets];
    locks = new pthread_mutex_t[numBuckets];

    // init buckets to null
    for (int i = 0; i < numBuckets; i++) {
        buckets[i] = nullptr;

        int res = -1;
        while (res != 0) {
            res = pthread_mutex_init(&locks[i], nullptr);
        }
    }
}

Map::~Map() {
    for (int i = 0; i < numBuckets; i++) {
        while (buckets[i] != nullptr) {
            remove(buckets[i]->key);
        }
        pthread_mutex_destroy(&locks[i]);
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

    lock(hashValue);

    Node* head = buckets[hashValue];

    if (keyInBucket(key, head)) {
        unlock(hashValue);
        return false;
    }

    Node* newNode = new Node(key, value);

    if (head == nullptr) {
        buckets[hashValue] = newNode;
        unlock(hashValue);
        return true;
    }

    newNode->next = head;
    buckets[hashValue] = newNode;
    unlock(hashValue);
    return true;
}

string Map::lookup(int key) {
    int hashValue = hash(key);

    lock(hashValue);
    Node* head = buckets[hashValue];

    for (Node* node = head; node != nullptr; node = node->next) {
        if (node->key == key) {
            unlock(hashValue);
            return node->value;
        }
    }

    unlock(hashValue);
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
    lock(hashValue);
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
            unlock(hashValue);
            return true;
        } else {
            prevNode = currNode;
            currNode = currNode->next;
        }
    }

    unlock(hashValue);
    return false;
}