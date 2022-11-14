#include "LinkedList.h"

class Map {
  private:
    const int SIZE = 1024;
    int bucketCount;
    LinkedList** buckets;
    int hash(int);
    int hash(Entry*);

  public:
    Map();
    ~Map();
    bool insert(Entry*);
    bool remove(Entry*);
    bool search(Entry*);
};

Map::Map() {
    bucketCount = SIZE / 10;
    buckets = new LinkedList*[bucketCount];
}

int Map::hash(int value) { return value % bucketCount; }
int Map::hash(Entry* entry) { return hash(entry->key); }

bool Map::insert(Entry* entry) {
    int hashValue = hash(entry);

    if (buckets[hashValue]->contains(entry)) {
        return false;
    }

    buckets[hashValue]->insert(entry);
    return true;
}

bool Map::search(Entry* entry) {
    int hashValue = hash(entry);

    return buckets[hashValue]->contains(entry);
}

bool Map::remove(Entry* entry) {
    int hashValue = hash(entry);

    if (!buckets[hashValue]->contains(entry)) {
        return false;
    }

    buckets[hashValue]->remove(entry);
    return true;
}