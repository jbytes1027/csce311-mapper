#include <string>

#include "Map.h"
#include "Semaphore.h"

using namespace std;

class ConcurrentMap : Map {
  private:
    // Array of sems, one for each bucket
    sem_t* sems;

    void lock(int bucket);

    void unlock(int bucket);

    void signal(sem_t*);

    int numCyclesToSleepPerOpp;

  public:
    ConcurrentMap(int numBuckets = 1000, int oppPaddingCycles = 0);

    ~ConcurrentMap();

    bool insertAndPost(int, string, sem_t*);

    bool removeAndPost(int, sem_t*);

    string lookupAndPost(int, sem_t*);
};
