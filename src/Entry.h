using namespace std;
#include <string>

#ifndef ENTRY_H
#define ENTRY_H

class Entry {
  public:
    Entry(int key, string value);
    int Print();
    int key;
    string value;
};

#endif