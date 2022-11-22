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
    long unsigned int oppToOutputIndex;
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
            // wakeup next consumer
            post(&state->semSignalOppStarted);
            // signal done
            post(&state->semConsumersDone);
            return 0;
        }

        // store snapshot of index
        long unsigned int threadOutIndex = state->currOppIndex;

        // increase opp index
        state->currOppIndex++;

        string output = state->map->executeLineAndPost(
            lineToExecute, &state->semSignalOppStarted);

        // wait until turn to output by moving through every thread and
        // seeing if it is their turn to output
        while (true) {
            wait(&state->semLockOut);
            // if its this threads turn to output
            if (threadOutIndex == state->oppToOutputIndex) {
                // flush output
                *(state->outputBuffer) << output;
                // increment flush turn
                state->oppToOutputIndex++;
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
    state.oppToOutputIndex = 0;

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
    cout << "Executing File\n";
    stringstream outputBuffer = executeStream(&outputStream);
    cout << "Writing output to disk\n";
    write(&outputBuffer, pathOutput);
}