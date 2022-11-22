using namespace std;

#include "Map.h"

#include <pthread.h>
#include <semaphore.h>

#include <iostream>
#include <sstream>
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

// in map to minimize lock time
// parse a line to execute and signal back to producer
string Map::executeLineAndPost(string line, sem_t* semSignalOppStarted) {
    // get key
    int keyStart = 2;
    int keyEnd = keyStart;
    // move forward till end of line (for L or D) or space (for I)
    for (long unsigned int i = keyStart; i <= line.length(); i++) {
        keyEnd = i;
        if (i != line.length() && line.at(i) == ' ') {
            break;
        }
    }
    int keyLen = keyEnd - keyStart;
    string keyString = line.substr(keyStart, keyLen);
    int key = stoi(keyString);

    int hashValue = hash(key);
    lock(hashValue);
    signal(semSignalOppStarted);

    stringstream out;

    char action = line.at(0);
    if (action == 'D') {
        bool success = remove(key);

        if (success) {
            out << "[Success] removed " << key << "\n";
        } else {
            out << "[Error] failed to remove " << key << ": value not found\n";
        }
    } else if (action == 'L') {
        string value = lookup(key);

        if (value != "") {
            out << "[Success] Found \"" << value << "\" from key " << key
                << "\n";
        } else {
            out << "[Error] failed to locate " << key << "\n";
        }
    } else if (action == 'I') {
        int valueStart = keyEnd + 2;
        int valueEnd = line.length() - 1;
        int valueLen = valueEnd - valueStart;
        string value = line.substr(valueStart, valueLen);

        bool success = insert(key, value);

        if (success) {
            out << "[Success] inserted " << value << " at " << key << "\n";
        } else {
            out << "[Error] failed to insert " << key << " at " << value
                << "\n";
        }
    }

    unlock(hashValue);
    return out.str();
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
