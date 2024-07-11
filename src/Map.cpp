#include "Map.h"

#include <semaphore.h>

#include <iostream>
#include <string>

Node::Node(int key, string value) {
    this->key = key;
    this->value = value;
    next = nullptr;
}

Map::Map(int numBuckets, int oppPaddingCycles) {
    this->numCyclesToSleepPerOpp = oppPaddingCycles;
    this->numBuckets = numBuckets;
    buckets = new Node*[numBuckets];
    sems = new sem_t[numBuckets];

    // Init buckets to nullptr and their sems to 1
    for (int i = 0; i < numBuckets; i++) {
        buckets[i] = nullptr;

        while (sem_init(&sems[i], 0, 1) != 0) {
            cout << "Error initializing sem\n";
        }
    }
}

Map::~Map() {
    // For every bucket, delete its nodes and its sem
    for (int i = 0; i < numBuckets; i++) {
        while (buckets[i] != nullptr) {
            remove(buckets[i]->key);
        }

        sem_destroy(&sems[i]);
    }

    delete buckets;
    delete sems;
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
    // Stores the previously visited node so that when the current node is
    // removed, the previous node's next pointer can be updated
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

void Map::lock(int bucket) { sem_wait(&sems[bucket]); }

void Map::unlock(int bucket) { sem_post(&sems[bucket]); }

// Alias for sem_post with error checking
void Map::signal(sem_t* semSignal) {
    while (sem_post(semSignal) == -1) {
        cout << "Error posting sem\n";
    }
}

// Spin to demonstrate scaling
void spin(int numCycles) { for (int i = 0; i < numCycles; i++); }

bool Map::concurrentInsertAndPost(int key, string value, sem_t* semOppStarted) {
    int bucket = hash(key);

    lock(bucket);
    // Tell caller opp has started
    signal(semOppStarted);
    spin(this->numCyclesToSleepPerOpp);
    bool result = insert(key, value);
    unlock(bucket);
    return result;
}

string Map::concurrentLookupAndPost(int key, sem_t* semOppStarted) {
    int bucket = hash(key);

    lock(bucket);
    // Tell caller opp has started
    signal(semOppStarted);
    spin(numCyclesToSleepPerOpp);
    string result = lookup(key);
    unlock(bucket);
    return result;
}

bool Map::concurrentRemoveAndPost(int key, sem_t* semOppStarted) {
    int bucket = hash(key);

    lock(bucket);
    // Tell caller opp has started
    signal(semOppStarted);
    spin(numCyclesToSleepPerOpp);
    bool result = remove(key);
    unlock(bucket);
    return result;
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
