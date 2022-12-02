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

// shared state for consumers and producers
struct mapper_shared_state_t {
    Map* map;
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

// consumer that consumes lines given from producer and outputs the result
void* consumeLineThread(void* args) {
    mapper_shared_state_t* state = (mapper_shared_state_t*)args;

    while (true) {
        string lineToExecute;

        // READ INPUT
        // lock to read one input line at a time
        wait(&state->semLockRead);
        // store line
        getline(*state->inputBuffer, lineToExecute);
        // store snapshot of index
        long unsigned int currOppIndex = state->currOppReadIndex;
        // increase opp index
        state->currOppReadIndex++;
        post(&state->semLockRead);
        // END READ INPUT

        // if no lines left to read
        if (lineToExecute == "") {
            // signal done
            post(&state->semConsumersDone);
            // exit
            return 0;
        }

        // PARSE KEY
        int keyStart = 2;
        int keyEnd = keyStart;
        // move forward till end of line (for Lookup or Delete)
        // or space (for Insert)
        for (long unsigned int i = keyStart; i <= lineToExecute.length(); i++) {
            keyEnd = i;
            if (i != lineToExecute.length() && lineToExecute.at(i) == ' ') {
                break;
            }
        }
        int keyLen = keyEnd - keyStart;
        string keyString = lineToExecute.substr(keyStart, keyLen);
        int key = stoi(keyString);
        // END PARSE KEY

        stringstream outputLine;
        char action = lineToExecute.at(0);

        // EXECUTE OPP
        // lock to ensure order of execution: unlocked in map opp call once opp
        // has started and map has internal lock
        // wait until turn to execute by moving through every thread and
        // seeing if it is their turn to output
        while (true) {
            wait(&state->semLockScheduleOpp);
            // if its this threads turn to execute; keep lock
            if (currOppIndex == state->currOppExecuteIndex) break;
            post(&state->semLockScheduleOpp);  // cycle through waiting
        }

        // inc because curr index is about to execute (and locked until it does)
        state->currOppExecuteIndex++;

        // run action on map and wait until the map has started the opp
        // then release the execution order enforcing lock
        // then figure out what to output
        if (action == 'D') {
            bool success = state->map->concurrentRemoveAndPost(
                key, &state->semLockScheduleOpp);

            if (success) {
                outputLine << "[Success] removed " << key << "\n";
            } else {
                outputLine << "[Error] failed to remove " << key
                           << ": value not found\n";
            }
        } else if (action == 'L') {
            string value = state->map->concurrentLookupAndPost(
                key, &state->semLockScheduleOpp);

            if (value != "") {
                outputLine << "[Success] Found \"" << value << "\" from key "
                           << key << "\n";
            } else {
                outputLine << "[Error] failed to locate " << key << "\n";
            }
        } else if (action == 'I') {
            int valueStart = keyEnd + 2;
            int valueEnd = lineToExecute.length() - 1;
            int valueLen = valueEnd - valueStart;
            string value = lineToExecute.substr(valueStart, valueLen);

            bool success = state->map->concurrentInsertAndPost(
                key, value, &state->semLockScheduleOpp);

            if (success) {
                outputLine << "[Success] inserted " << value << " at " << key
                           << "\n";
            } else {
                outputLine << "[Error] failed to insert " << key << " at "
                           << value << "\n";
            }
        }
        // END EXECUTE OPP

        // WRITE OUTPUT
        // lock to ensure order of output
        // wait until turn to output by moving through every thread and
        // seeing if it is their turn to output
        while (true) {
            wait(&state->semLockOut);
            // if its this threads turn to output; keep lock
            if (currOppIndex == state->oppToOutputIndex) break;
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
stringstream executeStream(stringstream* streamInput) {
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

    sem_init(&state.semConsumersDone, 0, 0);
    sem_init(&state.semLockScheduleOpp, 0, 1);
    sem_init(&state.semLockOut, 0, 1);
    sem_init(&state.semLockRead, 0, 1);
    state.map = new Map();
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