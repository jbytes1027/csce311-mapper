#include <semaphore.h>

#include <string>

#include "Map.h"

struct mapper_state_t;

void post(sem_t*);

void wait(sem_t*);

string executeLineAndPost(string line, Map* map, sem_t* semSignalOppStarted);

void* consumeLineThread(void* uncastArgs);

void write(stringstream* stream, string pathOutput);

stringstream executeStream(stringstream* streamInput);

stringstream executeStream(stringstream* streamInput, Map* map);

void executeFile(string pathInput, string pathOutput);
