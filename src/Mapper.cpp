using namespace std;
#include <semaphore.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>  // for sleeping

#include "Map.h"

void post(sem_t* sem) {
    // try to post until success
    while (sem_post(sem) == -1) {
        cout << "Error posting sem\n";
    }
}

void wait(sem_t* sem) {
    // try to wait until success
    while (sem_wait(sem) == -1) {
        cout << "Error waiting sem\n";
    }
}

// parse a line to execute and signal back to producer
string executeLineAndPost(string line, Map* map, sem_t* semSignalOppStarted) {
    stringstream out;

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

// shared state for consumers and producers
struct mapper_shared_state_t {
    Map* map;
    sem_t semOutputLineAvailable;
    // used to automically output from consumer
    sem_t semLockOut;
    sem_t semSignalOut;
    // tracks how many threads are running
    sem_t semConsumersWaiting;
    // tracks how many threads have stopped running
    sem_t semConsumersDone;
    // used to tell producer that the scheduled opperation has started
    sem_t semSignalOppStarted;
    // tracks which line the producer is producing
    long unsigned int currOppIndex;
    // tracks which opperation to output next
    long unsigned int oppToOutput;
    stringstream* inputBuffer;
    stringstream* outputBuffer;
};

// consumer that consumes lines given from producer and outputs the result
void* consumeLineThread(void* args) {
    mapper_shared_state_t* state = (mapper_shared_state_t*)args;

    while (true) {
        wait(&state->semSignalOppStarted);

        string lineToExecute;
        getline(*state->inputBuffer, lineToExecute);

        if (lineToExecute == "") {
            post(&state->semConsumersDone);
            post(&state->semSignalOppStarted);
            return 0;
        }

        // store snapshot of index
        long unsigned int threadOutIndex = state->currOppIndex;

        // increase opp index
        state->currOppIndex++;

        string output = executeLineAndPost(lineToExecute, state->map,
                                           &state->semSignalOppStarted);

        // wait until turn to output by moving through every thread and
        // seeing if it is their turn to output
        while (true) {
            wait(&state->semLockOut);
            // if its this threads turn to output
            if (threadOutIndex == state->oppToOutput) {
                // flush output
                *(state->outputBuffer) << output;
                // increment flush turn
                state->oppToOutput++;
                // signal thread is done consuming line
                post(&state->semConsumersWaiting);
                post(&state->semLockOut);
                break;
            }
            post(&state->semLockOut);
        }
    }
}

void write(stringstream* stream, string pathOutput) {
    ofstream fileOutput(pathOutput, ifstream::out);
    fileOutput << stream->rdbuf();
    fileOutput.flush();
    fileOutput.close();
}

// runs input stream line by line and returns output
stringstream executeStream(stringstream* streamInput) {
    int numConsumers;

    mapper_shared_state_t state;

    state.inputBuffer = streamInput;
    stringstream outputBuffer;
    state.outputBuffer = &outputBuffer;

    string threadsInfoLine;
    getline(*streamInput, threadsInfoLine);
    numConsumers =
        stoi(threadsInfoLine.substr(2, threadsInfoLine.length() - 2));
    *state.outputBuffer << "Using " << numConsumers << " threads to consume\n";

    sem_init(&state.semConsumersWaiting, 0, numConsumers);
    sem_init(&state.semConsumersDone, 0, 0);
    sem_init(&state.semSignalOppStarted, 0, 1);
    sem_init(&state.semSignalOut, 0, 0);
    sem_init(&state.semLockOut, 0, 1);
    sem_init(&state.semOutputLineAvailable, 0, 0);
    state.map = new Map();
    state.currOppIndex = 0;
    state.oppToOutput = 0;

    // start numThreads consumer threads
    for (int i = 0; i < numConsumers; i++) {
        pthread_t thread;
        int status =
            pthread_create(&thread, nullptr, consumeLineThread, &state);
        if (status != 0) {
            cout << "Error starting thread\n";
            return outputBuffer;
        }
    }

    while (!streamInput->eof()) this_thread::sleep_for(chrono::milliseconds(5));

    // wait for threads to finish outputing
    int consumersWaiting = -1;
    do {
        sem_getvalue(&state.semConsumersWaiting, &consumersWaiting);
        this_thread::sleep_for(chrono::milliseconds(5));
    } while (consumersWaiting < numConsumers);

    // wake all threads so they can notice doneProducting and exit
    wait(&state.semConsumersWaiting);
    for (int i = 0; i < numConsumers; i++) {
        post(&state.semOutputLineAvailable);
    }

    // wait for threads to exit
    int consumersDone = -1;
    do {
        sem_getvalue(&state.semConsumersDone, &consumersDone);
        this_thread::sleep_for(chrono::milliseconds(5));
    } while (consumersDone < numConsumers);

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