#include <semaphore.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>  // for sleeping

#include "ConcurrentMap.h"

using namespace std;

// shared state for consumers and producers
struct mapper_shared_state_t {
    ConcurrentMap* map;
    sem_t semLockRead;
    // used to automically output from consumer
    sem_t semLockOut;
    // tracks how many threads have stopped running
    sem_t semConsumersDone;
    // used to tell producer that the scheduled opperation has started
    sem_t semLockScheduleOpp;
    // tracks which line the producer is producing
    long unsigned int currOppReadIndex;
    long unsigned int currOppExecuteIndex;
    // tracks which opperation to output next
    long unsigned int oppToOutputIndex;
    stringstream* inputBuffer;
    stringstream* outputBuffer;
};

enum operation_type_t {
    INSERT,
    LOOKUP,
    DELETE,
};

struct operation_t {
    operation_type_t type;
    int key;
    string value;
};

void readlineFromState(mapper_shared_state_t* state,
                       long unsigned int* readLineIndex, string* readLine) {
    wait(&state->semLockRead);
    getline(*state->inputBuffer, *readLine);
    // store snapshot of index
    *readLineIndex = state->currOppReadIndex;
    // increase opp index
    state->currOppReadIndex++;
    post(&state->semLockRead);
}

operation_t parse(string line) {
    operation_t opp;

    int keyStart = 2;
    int keyEnd = keyStart;
    // move forward till end of line (for Lookup or Delete)
    // or space (for Insert)
    for (long unsigned int i = keyStart; i <= line.length(); i++) {
        keyEnd = i;
        if (i != line.length() && line.at(i) == ' ') {
            break;
        }
    }
    int keyLen = keyEnd - keyStart;
    string keyString = line.substr(keyStart, keyLen);
    opp.key = stoi(keyString);

    switch (line.at(0)) {
        case 'I':
            opp.type = INSERT;
            break;
        case 'L':
            opp.type = LOOKUP;
            break;
        case 'D':
            opp.type = DELETE;
            break;
    }

    if (opp.type == INSERT) {
        int valueStart = keyEnd + 2;
        int valueEnd = line.length() - 1;
        int valueLen = valueEnd - valueStart;
        opp.value = line.substr(valueStart, valueLen);
    }

    return opp;
}

inline void executeOppAndPost(mapper_shared_state_t* state, operation_t opp,
                              stringstream* outputLine) {
    if (opp.type == DELETE) {
        bool success =
            state->map->removeAndPost(opp.key, &state->semLockScheduleOpp);

        if (success) {
            *outputLine << "[Success] removed " << opp.key << "\n";
        } else {
            *outputLine << "[Error] failed to remove " << opp.key
                        << ": value not found\n";
        }
    } else if (opp.type == LOOKUP) {
        string value =
            state->map->lookupAndPost(opp.key, &state->semLockScheduleOpp);

        if (value != "") {
            *outputLine << "[Success] Found \"" << value << "\" from key "
                        << opp.key << "\n";
        } else {
            *outputLine << "[Error] failed to locate " << opp.key << "\n";
        }
    } else if (opp.type == INSERT) {
        bool success = state->map->insertAndPost(opp.key, opp.value,
                                                 &state->semLockScheduleOpp);

        if (success) {
            *outputLine << "[Success] inserted " << opp.value << " at "
                        << opp.key << "\n";
        } else {
            *outputLine << "[Error] failed to insert " << opp.key << " at "
                        << opp.value << "\n";
        }
    }
}

// Consumer that consumes lines given from producer and outputs the result
void* consumeLineThread(void* args) {
    mapper_shared_state_t* state = (mapper_shared_state_t*)args;

    while (true) {
        long unsigned int readIndex;
        string readLine;

        readlineFromState(state, &readIndex, &readLine);

        // If no lines left to read, signal done and stop
        if (readLine == "") {
            post(&state->semConsumersDone);
            return 0;
        }

        operation_t opp = parse(readLine);

        stringstream outputLine;

        // EXECUTE OPP
        // lock to ensure order of execution: unlocked in map opp call once opp
        // has started and map has internal lock
        // wait until turn to execute by moving through every thread and
        // seeing if it is their turn to output
        while (true) {
            wait(&state->semLockScheduleOpp);
            // if its this threads turn to execute; keep lock
            if (readIndex == state->currOppExecuteIndex) break;
            post(&state->semLockScheduleOpp);  // cycle through waiting
        }

        // inc because curr index is about to execute (and locked until it does)
        state->currOppExecuteIndex++;

        // run action on map and wait until the map has started the opp
        // then release the execution order enforcing lock
        // then figure out what to output
        executeOppAndPost(state, opp, &outputLine);

        // END EXECUTE OPP

        // WRITE OUTPUT
        // lock to ensure order of output
        // wait until turn to output by moving through every thread and
        // seeing if it is their turn to output
        while (true) {
            wait(&state->semLockOut);
            // if its this threads turn to output; keep lock
            if (readIndex == state->oppToOutputIndex) break;
            post(&state->semLockOut);  // cycle through waiting
        }

        // flush output
        *(state->outputBuffer) << outputLine.str();
        // increment flush turn
        state->oppToOutputIndex++;
        post(&state->semLockOut);
        // END WRITE OUTPUT
    }
}

void write(stringstream* stream, string pathOutput) {
    ofstream fileOutput(pathOutput, ifstream::out);
    fileOutput << stream->rdbuf();
    fileOutput.flush();
    fileOutput.close();
}

// runs input stream and returns output in stringstream buffer
// map argument for testing
stringstream executeStream(stringstream* streamInput, ConcurrentMap* map) {
    int numConsumers;

    mapper_shared_state_t state;

    state.inputBuffer = streamInput;
    // init the output buffer
    stringstream outputBuffer;
    state.outputBuffer = &outputBuffer;

    string threadsInfoLine;
    // get the first line which contains the number of threads to use
    getline(*streamInput, threadsInfoLine);
    // parse the number of consumers to use
    numConsumers =
        stoi(threadsInfoLine.substr(2, threadsInfoLine.length() - 2));
    *state.outputBuffer << "Using " << numConsumers << " threads to consume\n";

    init(&state.semConsumersDone, 0);
    init(&state.semLockScheduleOpp, 1);
    init(&state.semLockOut, 1);
    init(&state.semLockRead, 1);
    state.map = map;
    state.currOppReadIndex = 0;
    state.currOppExecuteIndex = 0;
    state.oppToOutputIndex = 0;

    // create all consumer threads
    for (int i = 0; i < numConsumers; i++) {
        pthread_t thread;
        int status =
            pthread_create(&thread, nullptr, consumeLineThread, &state);
        if (status != 0) {
            cout << "Error starting thread\n";
            return outputBuffer;
        }
    }

    int consumersDone = -1;
    // check every x ms to see if all threads finished
    do {
        sem_getvalue(&state.semConsumersDone, &consumersDone);
        this_thread::sleep_for(chrono::milliseconds(5));
        // while not all threads done
    } while (consumersDone < numConsumers);

    delete state.map;
    return outputBuffer;
}

stringstream executeStream(stringstream* streamInput) {
    return executeStream(streamInput, new ConcurrentMap());
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
    cout << "Executing File\n";
    stringstream outputBuffer = executeStream(&outputStream);
    cout << "Writing output to disk\n";
    write(&outputBuffer, pathOutput);
}
