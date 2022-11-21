using namespace std;
#include <semaphore.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "Map.h"

void post(sem_t* sem) {
    while (sem_post(sem) == -1) {
        cout << "Error posting sem\n";
    }
}

void wait(sem_t* sem) {
    while (sem_wait(sem) == -1) {
        cout << "Error waiting sem\n";
    }
}

string executeLineOppAndSignal(string line, Map* map,
                               sem_t* semSignalOppStarted) {
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
        bool success = map->concurrentRemoveAndPost(key, semSignalOppStarted);

        if (success) {
            out << "[Success] removed " << key << "\n";
        } else {
            out << "[Error] failed to remove " << key << ": value not found\n";
        }
    } else if (action == 'L') {
        string value = map->concurrentLookupAndPost(key, semSignalOppStarted);

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
            map->concurrentInsertAndPost(key, value, semSignalOppStarted);

        if (success) {
            out << "[Success] inserted " << value << " at " << key << "\n";
        } else {
            out << "[Error] failed to insert " << key << " at " << value
                << "\n";
        }
    }

    return out.str();
}

struct mapper_shared_state_t {
    Map* map;
    sem_t* semOutputLineAvailable;
    sem_t* semLockOut;
    sem_t* semSignalOut;
    sem_t* semThreadsWaiting;
    sem_t* semSignalOppStarted;
    string currInputLine;
    long unsigned int outTurnIndex;
    long unsigned int flushOutThreadTurn;
    stringstream* outputBuffer;
};

void* consumeLineThread(void* uncastArgs) {
    mapper_shared_state_t* args = (mapper_shared_state_t*)uncastArgs;

    while (true) {
        wait(args->semOutputLineAvailable);  // wait for line to be produced
        // store snapshot of curr line and index
        long unsigned int threadOutIndex = args->outTurnIndex;

        string output = executeLineOppAndSignal(args->currInputLine, args->map,
                                                args->semSignalOppStarted);

        // wait until turn to output
        while (true) {
            wait(args->semLockOut);
            if (threadOutIndex == args->flushOutThreadTurn) {
                *(args->outputBuffer) << output;
                args->flushOutThreadTurn++;
                post(args->semThreadsWaiting);
                post(args->semLockOut);
                break;
            }
            post(args->semSignalOut);
            post(args->semLockOut);
            wait(args->semSignalOut);
        }
    }
}

void write(stringstream* stream, string pathOutput) {
    ofstream fileOutput(pathOutput, ifstream::out);
    fileOutput << stream->rdbuf();
    fileOutput.flush();
    fileOutput.close();
}

stringstream executeStream(stringstream* streamInput) {
    int numThreads;

    mapper_shared_state_t state;
    stringstream outputBuffer;
    state.outputBuffer = &outputBuffer;
    sem_t semThreadsWaiting;
    state.semThreadsWaiting = &semThreadsWaiting;
    sem_t semSignalOppStarted;
    state.semSignalOppStarted = &semSignalOppStarted;
    sem_t semSignalOut;
    state.semSignalOut = &semSignalOut;
    sem_t semLockOut;
    state.semLockOut = &semLockOut;
    sem_t semOutputLineAvailable;
    state.semOutputLineAvailable = &semOutputLineAvailable;

    auto begin = chrono::high_resolution_clock::now();

    getline(*streamInput, state.currInputLine);
    numThreads =
        stoi(state.currInputLine.substr(2, state.currInputLine.length() - 2));
    *state.outputBuffer << "Using " << numThreads << " threads\n";

    sem_init(&semThreadsWaiting, 0, numThreads);
    sem_init(&semSignalOppStarted, 0, 0);
    sem_init(&semSignalOut, 0, 0);
    sem_init(&semLockOut, 0, 1);
    sem_init(&semOutputLineAvailable, 0, 0);
    state.map = new Map();
    state.outTurnIndex = 0;
    state.flushOutThreadTurn = 0;

    for (int i = 0; i < numThreads; i++) {
        pthread_t thread;
        int status =
            pthread_create(&thread, nullptr, consumeLineThread, &state);
        if (status != 0) {
            cout << "Error starting thread\n";
            return outputBuffer;
        }
    }

    // produce lines
    while (getline(*streamInput, state.currInputLine)) {
        wait(&semThreadsWaiting);
        post(&semOutputLineAvailable);  // wake thread
        wait(&semSignalOppStarted);     // wait until map opperation has
                                        // started to ensure order and allow
                                        // thread to get line and index
        state.outTurnIndex++;
        post(&semSignalOut);
    }

    auto end = chrono::high_resolution_clock::now();
    cout << chrono::duration_cast<chrono::nanoseconds>(end - begin).count()
         << "ns" << std::endl;

    delete state.map;
    return outputBuffer;
}

void executeFile(string pathInput, string pathOutput) {
    ifstream fileInput(pathInput, ifstream::in);

    if (!fileInput.is_open()) {
        cout << "Error opening file\n";
        return;
    }

    cout << "Loading File\n";
    stringstream outputStream;
    outputStream << fileInput.rdbuf();
    stringstream outputBuffer = executeStream(&outputStream);
    cout << "Writing output to disk\n";
    write(&outputBuffer, pathOutput);
}