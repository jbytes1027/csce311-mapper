using namespace std;
#include <semaphore.h>

#include <chrono>  //for sleeping
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>  //for sleeping

#include "Map.h"

int numThreads = 1;
Map* map;
sem_t semThreads;
sem_t semSignalOppStarted;
sem_t semSignalOut;
sem_t semLockOut;
long unsigned int outIndex = 0;

string executeLineOpp(string line) {
    stringstream out;

    int keyStart = 2;
    int keyEnd = keyStart;
    for (long unsigned int i = keyStart; i <= line.length(); i++) {
        keyEnd = i;
        if (i != line.length() && line.at(i) == ' ') {
            break;
        }
    }
    int keyLen = keyEnd - keyStart;
    string keyString = line.substr(keyStart, keyLen);
    int key = stoi(keyString);

    char action = line.at(0);
    if (action == 'D') {
        bool success = map->concurrentRemoveAndPost(key, &semSignalOppStarted);

        if (success) {
            out << "[Success] removed " << key << "\n";
        } else {
            out << "[Error] failed to remove " << key << ": value not found\n";
        }
    } else if (action == 'L') {
        string value = map->concurrentLookupAndPost(key, &semSignalOppStarted);

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

        bool success =
            map->concurrentInsertAndPost(key, value, &semSignalOppStarted);

        if (success) {
            out << "[Success] inserted " << value << " at " << key << "\n";
        } else {
            out << "[Error] failed to insert " << key << " at " << value
                << "\n";
        }
    }

    return out.str();
}

void* chompLineThread(void* arg) {
    long unsigned int threadOutIndex = outIndex + 1;
    string line = *((string*)arg);
    string output = executeLineOpp(line);
    // TODO wait till i=print index - cant spin; will block everything else
    while (true) {
        sem_wait(&semLockOut);
        if (threadOutIndex == outIndex) {
            sem_post(&semLockOut);
            break;
        }
        sem_post(&semSignalOut);
        sem_post(&semLockOut);
        sem_wait(&semSignalOut);
    }
    cout << output;
    sem_post(&semThreads);
    return 0;
}

void executeFile(string path) {
    ifstream file(path, ifstream::in);
    string line;

    if (!file.is_open()) {
        cout << "Error opening file\n";
        return;
    }

    getline(file, line);
    numThreads = stoi(line.substr(2, line.length() - 2));
    numThreads = 1;
    cout << "Using " << numThreads << " threads\n";

    sem_init(&semThreads, 0, numThreads);
    sem_init(&semSignalOppStarted, 0, 0);
    sem_init(&semSignalOut, 0, 0);
    sem_init(&semLockOut, 0, 1);
    map = new Map();

    while (getline(file, line)) {
        sem_wait(&semThreads);
        pthread_t thread;
        pthread_create(&thread, nullptr, chompLineThread, &line);
        sem_wait(&semSignalOppStarted);
        outIndex++;
        sem_post(&semSignalOut);
    }

    delete map;
}

void sleep(int ms) { this_thread::sleep_for(chrono::milliseconds(ms)); }

int main(int argc, char** argv) {
    if (argc > 1) {
        executeFile(argv[1]);
    } else {
        cout << "Missing filename\nUsage: mapper [INPUT FILE...] [OUTPUT "
                "FILE...]\n";
    }
}