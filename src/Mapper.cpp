#include <semaphore.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "ConcurrentMap.h"

using namespace std;

// Shared state for consumers and producers
struct mapper_shared_state_t {
    ConcurrentMap* map;

    sem_t semLockRead;

    // Used to automatically output from consumer
    sem_t semLockOut;

    int remainingConsumers;

    sem_t semRemainingConsumers;

    // Unlocks when all consumers are done
    sem_t semAllConsumersDone;

    // Used to tell producer that the scheduled operation has started
    sem_t semLockScheduleOpp;

    // Tracks which line the producer is producing
    long unsigned int currOppReadIndex;

    long unsigned int currOppExecuteIndex;

    // Tracks which operation to output next
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

inline void readLine(mapper_shared_state_t* state, long unsigned int* lineReadIndex,
                        string* lineRead) {
    while (sem_trywait(&state->semLockRead) != 0);
    getline(*state->inputBuffer, *lineRead);
    // Store a snapshot of the index
    *lineReadIndex = state->currOppReadIndex;
    state->currOppReadIndex++;
    post(&state->semLockRead);
}

inline void parse(string line, operation_t* opp) {
    int keyStart = 2;
    int keyEnd = keyStart;
    // Move forward until the end of the line (for Lookup or Delete) or space (for Insert)
    for (long unsigned int i = keyStart; i <= line.length(); i++) {
        keyEnd = i;
        if (i != line.length() && line.at(i) == ' ') {
            break;
        }
    }
    int keyLen = keyEnd - keyStart;
    string keyString = line.substr(keyStart, keyLen);
    opp->key = stoi(keyString);

    switch (line.at(0)) {
        case 'I':
            opp->type = INSERT;
            break;
        case 'L':
            opp->type = LOOKUP;
            break;
        case 'D':
            opp->type = DELETE;
            break;
    }

    if (opp->type == INSERT) {
        // Get the value to insert
        int valueStart = keyEnd + 2;
        int valueEnd = line.length() - 1;
        int valueLen = valueEnd - valueStart;
        opp->value = line.substr(valueStart, valueLen);
    }
}

// Run an operation on map and return the output
inline void executeOperation(mapper_shared_state_t* state, operation_t opp,
                             string* outputLine) {
    // Lock to ensure order of execution.
    // Lock is unlocked from map when it has an internal lock
    wait(&state->semLockScheduleOpp);
    // Increment so that next operation can run after lock is released
    state->currOppExecuteIndex++;

    if (opp.type == DELETE) {
        bool success = state->map->removeAndPost(opp.key, &state->semLockScheduleOpp);

        if (success) {
            *outputLine + "[Success] removed " + to_string(opp.key) + "\n";
        } else {
            *outputLine + "[Error] failed to remove " + to_string(opp.key) + ": value not found\n";
        }
    } else if (opp.type == LOOKUP) {
        string value = state->map->lookupAndPost(opp.key, &state->semLockScheduleOpp);

        if (value != "") {
            *outputLine + "[Success] Found \"" + value + "\" from key " + to_string(opp.key) + "\n";
        } else {
            *outputLine + "[Error] failed to locate " + to_string(opp.key) + "\n";
        }
    } else if (opp.type == INSERT) {
        bool success = state->map->insertAndPost(opp.key, opp.value, &state->semLockScheduleOpp);

        if (success) {
            *outputLine + "[Success] inserted " + opp.value + " at " + to_string(opp.key) + "\n";
        } else {
            *outputLine + "[Error] failed to insert " + to_string(opp.key) + " at " + opp.value + "\n";
        }
    }
}

inline void signalConsumerDone(mapper_shared_state_t*& state) {
    wait(&state->semRemainingConsumers);

    state->remainingConsumers -= 1;
    if (state->remainingConsumers == 0) {
        // Tells producer consumers are finished so wakeup
        post(&state->semAllConsumersDone);
    }

    post(&state->semRemainingConsumers);
}

inline void writeToOutput(mapper_shared_state_t*& state, string outputLine) {
    wait(&state->semLockOut);

    *(state->outputBuffer) << outputLine;
    state->oppToOutputIndex++;

    post(&state->semLockOut);
}

// Consumes lines produced by producer and outputs the result
void* consumeLineThread(void* args) {
    mapper_shared_state_t* state = (mapper_shared_state_t*)args;
    string outputLine;
    operation_t opp;

    while (true) {
        long unsigned int lineReadIndex;
        string lineRead;
        readLine(state, &lineReadIndex, &lineRead);

        // If no lines left to read
        if (lineRead == "") {
            signalConsumerDone(state);
            return 0;
        }

        // Wait for right turn to execute
        while (lineReadIndex != state->currOppExecuteIndex);
        parse(lineRead, &opp);
        executeOperation(state, opp, &outputLine);

        // Wait for right turn to output
        while (lineReadIndex != state->oppToOutputIndex);
        writeToOutput(state, outputLine);
    }
}

void write(stringstream* stream, string pathOutput) {
    ofstream fileOutput(pathOutput, ifstream::out);
    fileOutput << stream->rdbuf();
    fileOutput.flush();
    fileOutput.close();
}

mapper_shared_state_t initState(stringstream* streamInput, ConcurrentMap* map,
                                stringstream* outputBuffer) {
    mapper_shared_state_t state;

    state.inputBuffer = streamInput;
    state.map = map;
    state.outputBuffer = outputBuffer;

    string threadsInfoLine;
    // Get the first line which contains the number of threads to use
    getline(*streamInput, threadsInfoLine);
    // Parse the number of consumers to use
    state.remainingConsumers = stoi(threadsInfoLine.substr(2, threadsInfoLine.length() - 2));
    *state.outputBuffer << "Using " << state.remainingConsumers << " threads to consume\n";

    init(&state.semRemainingConsumers, 1);
    init(&state.semAllConsumersDone, 0);
    init(&state.semLockScheduleOpp, 1);
    init(&state.semLockOut, 1);
    init(&state.semLockRead, 1);
    state.currOppReadIndex = 0;
    state.currOppExecuteIndex = 0;
    state.oppToOutputIndex = 0;

    return state;
}

// Runs the input stream and returns output in stringstream buffer
// Argument map is for testing
stringstream executeStream(stringstream* streamInput, ConcurrentMap* map) {
    stringstream outputBuffer;
    mapper_shared_state_t state = initState(streamInput, map, &outputBuffer);

    for (int i = state.remainingConsumers; i > 0; i--) {
        pthread_t thread;
        int status = pthread_create(&thread, nullptr, consumeLineThread, &state);
        if (status != 0) {
            cout << "Error starting thread\n";
            return outputBuffer;
        }
    }

    sem_wait(&state.semAllConsumersDone);

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

    cout << "Loading file into memory\n";
    stringstream outputStream;
    outputStream << fileInput.rdbuf();

    cout << "Executing file\n";
    stringstream outputBuffer = executeStream(&outputStream);

    cout << "Writing output to disk\n";
    write(&outputBuffer, pathOutput);
}
