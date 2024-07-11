using namespace std;

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

    // init buckets to nullptr and their sem to 1
    for (int i = 0; i < numBuckets; i++) {
        buckets[i] = nullptr;

        int res = -1;
        while (res != 0) {
            res = sem_init(&sems[i], 0, 1);
        }
    }
}

Map::~Map() {
    // for every bucket
    for (int i = 0; i < numBuckets; i++) {
        // delete all the nodes in the bucket
        while (buckets[i] != nullptr) {
            remove(buckets[i]->key);
        }
        // delete the buckets sem
        sem_destroy(&sems[i]);
    }

    delete buckets;
    delete sems;
}

int Map::hash(int value) { return value % numBuckets; }

bool Map::keyInBucket(int key, Node* head) {
    // start at the head node of the bucket; move forwared until nullptr
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

    // if key already exists fail to insert
    if (keyInBucket(key, head)) return false;

    // new node to insert
    Node* newNode = new Node(key, value);

    // if the bucket is not empty:
    // point the new node to the existing bucket's head
    if (head != nullptr) newNode->next = head;

    // insert the new node at the head of the bucket
    buckets[hashValue] = newNode;
    return true;
}

string Map::lookup(int key) {
    int hashValue = hash(key);

    // start at the head node of the bucket; move forwared until nullptr
    for (Node* node = buckets[hashValue]; node != nullptr; node = node->next) {
        if (node->key == key) {
            return node->value;
        }
    }

    // nothing found
    return "";
}

bool Map::remove(int key) {
    int hashValue = hash(key);

    // stores the previously visited node so that when the current node is
    // removed, the previous node's next pointer can be updated
    Node* prevNode = nullptr;
    // start from the head of the bucket
    Node* currNode = buckets[hashValue];
    // while not at the end of the bucket
    while (currNode != nullptr) {
        if (currNode->key == key) {
            // update pointers around node
            // if not removing head
            if (prevNode != nullptr) {
                prevNode->next = currNode->next;
            } else {  // removing head
                buckets[hashValue] = currNode->next;
            }

            delete currNode;
            return true;
        } else {
            // move forward
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

// sleep without blocking; for demonstrating scaling; defaults to 0 cycles
void sleep(int numCycles) {
    for (int i = 0; i < numCycles; i++);
    return;
}

bool Map::concurrentInsertAndPost(int key, string value, sem_t* semOppStarted) {
    int hashValue = hash(key);
    lock(hashValue);
    // tell caller opp has started
    signal(semOppStarted);
    sleep(this->numCyclesToSleepPerOpp);
    bool result = insert(key, value);
    unlock(hashValue);
    return result;
}

string Map::concurrentLookupAndPost(int key, sem_t* semOppStarted) {
    int hashValue = hash(key);
    lock(hashValue);
    // tell caller opp has started
    signal(semOppStarted);
    sleep(numCyclesToSleepPerOpp);
    string result = lookup(key);
    unlock(hashValue);
    return result;
}

bool Map::concurrentRemoveAndPost(int key, sem_t* semOppStarted) {
    int hashValue = hash(key);
    lock(hashValue);
    // tell caller opp has started
    signal(semOppStarted);
    sleep(numCyclesToSleepPerOpp);
    bool result = remove(key);
    unlock(hashValue);
    return result;
}

// for debugging
void Map::printBuckets() {
    for (int i = 0; i < numBuckets; i++) {
        cout << i << ": ";
        printBucket(buckets[i]);
    }
}

// for debugging
void Map::printBucket(Node* head) {
    for (Node* node = head; node != nullptr; node = node->next) {
        cout << "(" << node->key << ", " << node->value << ") -> ";
    }
    cout << "\n";
}
