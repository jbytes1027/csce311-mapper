using namespace std;
#include <fstream>
#include <iostream>
#include <string>

#include "Map.h"

void executeFile(string path) {
    ifstream file(path, ifstream::in);

    if (!file.is_open()) {
        cout << "Error opening file";
    }

    Map* map;

    string line;
    while (getline(file, line)) {
        int keyStart = 2;
        int keyEnd = keyStart;
        for (int i = keyStart; i <= line.length(); i++) {
            keyEnd = i;
            if (i != line.length() && line.at(i) == ' ') {
                break;
            }
        }
        int keyLen = keyEnd - keyStart;
        string keyString = line.substr(keyStart, keyLen);
        int key = stoi(keyString);

        char action = line.at(0);
        if (action == 'N') {
            // Set thread number
            map = new Map(key);
            cout << "Using " << key << " threads\n";
        } else if (action == 'D') {
            bool success = map->remove(key);

            if (success) {
                cout << "[Success] removed " << key << "\n";
            } else {
                cout << "[Error] failed to remove " << key
                     << ": value not found\n";
            }
        } else if (action == 'L') {
            string value = map->lookup(key);

            if (value != "") {
                cout << "[Success] Found \"" << value << "\" from key " << key
                     << "\n";
            } else {
                cout << "[Error] failed to locate " << key << "\n";
            }
        } else if (action == 'I') {
            int valueStart = keyEnd + 2;
            int valueEnd = line.length() - 1;
            int valueLen = valueEnd - valueStart;
            string value = line.substr(valueStart, valueLen);

            bool success = map->insert(key, value);

            if (success) {
                cout << "[Success] inserted " << value << " at " << key << "\n";
            } else {
                cout << "[Error] failed to insert " << key << " at " << value
                     << "\n";
            }
        }

        map->printBuckets();
    }

    delete map;
}

int main(int argc, char** argv) {
    // Map* m = new Map();
    // m->insert(1, "3a");
    // m->insert(11, "3b");
    // m->lookup(1);
    // m->printBuckets();
    // m->remove(1);
    // m->lookup(1);
    // delete m;

    if (argc > 1) {
        executeFile(argv[1]);
    }
}