#include <string>

#include "ConcurrentMap.h"
#include "Semaphore.h"

struct mapper_state_t;

void* consumeLineThread(void* uncastArgs);

void write(stringstream* stream, string pathOutput);

stringstream executeStream(stringstream* streamInput);

stringstream executeStream(stringstream* streamInput, ConcurrentMap* map);

void executeFile(string pathInput, string pathOutput);
