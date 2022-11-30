using namespace std;

#include "Map.h"

#include <pthread.h>
#include <semaphore.h>

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
    sems = new sem_t[numBuckets];

    // init buckets to null
    for (int i = 0; i < numBuckets; i++) {
        buckets[i] = nullptr;

        int res = -1;
        while (res != 0) {
            res = sem_init(&sems[i], 0, 1);
        }
    }
}

Map::~Map() {
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

void Map::lock(int bucket) { sem_wait(&sems[bucket]); }

void Map::unlock(int bucket) { sem_post(&sems[bucket]); }

void Map::signal(sem_t* semSignal) {
    while (sem_post(semSignal) == -1) {
        cout << "Error posting sem\n";
    }
}

void sleep() {
    for (int i = 0; i < 10000; i++)
        ;
    return;
}

bool Map::concurrentInsertAndPost(int key, string value, sem_t* semOppStarted) {
    int hashValue = hash(key);
    lock(hashValue);
    signal(semOppStarted);
    sleep();
    bool result = insert(key, value);
    unlock(hashValue);
    return result;
}

string Map::concurrentLookupAndPost(int key, sem_t* semOppStarted) {
    int hashValue = hash(key);
    lock(hashValue);
    signal(semOppStarted);
    sleep();
    string result = lookup(key);
    unlock(hashValue);
    return result;
}

bool Map::concurrentRemoveAndPost(int key, sem_t* semOppStarted) {
    int hashValue = hash(key);
    lock(hashValue);
    signal(semOppStarted);
    sleep();
    bool result = remove(key);
    unlock(hashValue);
    return result;
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
