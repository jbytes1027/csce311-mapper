using namespace std;
#include "Entry.h"

#include <string>

Entry::Entry(int key, string value) {
    this->key = key;
    this->value = value;
}

int Entry::Print() { return 0; }