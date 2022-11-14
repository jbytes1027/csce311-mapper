using namespace std;
#include <fstream>
#include <iostream>
#include <string>

#include "LinkedList.h"

void executeFile(string path) {
    ifstream file(path);

    if (!file.is_open()) {
        cout << "Error opening file";
    }

    string line;
    while (getline(file, line)) {
        cout << line;
    }
}

int main() {
    LinkedList* ll = new LinkedList();
    Entry* e = new Entry(4, "string");
    ll->insert(e);
    ll->print();
    delete ll;
}