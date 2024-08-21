#include "ConcurrentMap.h"

#include "Semaphore.h"

ConcurrentMap::ConcurrentMap(int numBuckets, int oppPaddingCycles) : Map(numBuckets) {
    this->numCyclesToSleepPerOpp = oppPaddingCycles;
    sems = new sem_t[numBuckets];

    for (int i = 0; i < numBuckets; i++) {
        init(&sems[i], 1);
    }
}

ConcurrentMap::~ConcurrentMap() {
    for (int i = 0; i < numBuckets; i++) {
        sem_destroy(&sems[i]);
    }

    delete sems;
}

void ConcurrentMap::lock(int bucket) { wait(&sems[bucket]); }

void ConcurrentMap::unlock(int bucket) { post(&sems[bucket]); }

// Spin to demonstrate scaling
void spin(int numCycles) { for (int i = 0; i < numCycles; i++); }

bool ConcurrentMap::insertAndPost(int key, string value, sem_t* semOppStarted) {
    int bucket = hash(key);

    lock(bucket);
    // Tell caller opp has started
    post(semOppStarted);
    spin(this->numCyclesToSleepPerOpp);
    bool result = insert(key, value);
    unlock(bucket);
    return result;
}

string ConcurrentMap::lookupAndPost(int key, sem_t* semOppStarted) {
    int bucket = hash(key);

    lock(bucket);
    // Tell caller opp has started
    post(semOppStarted);
    spin(numCyclesToSleepPerOpp);
    string result = lookup(key);
    unlock(bucket);
    return result;
}

bool ConcurrentMap::removeAndPost(int key, sem_t* semOppStarted) {
    int bucket = hash(key);

    lock(bucket);
    // Tell caller opp has started
    post(semOppStarted);
    spin(numCyclesToSleepPerOpp);
    bool result = remove(key);
    unlock(bucket);
    return result;
}
