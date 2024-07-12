#include "Semaphore.h"

#include <iostream>

void post(sem_t* sem) {
    while (sem_post(sem) != 0) {
        std::cout << "Error posting sem\n";
    }
}

void wait(sem_t* sem) {
    while (sem_wait(sem) != 0) {
        std::cout << "Error waiting sem\n";
    }
}

void init(sem_t* sem, int value) {
    while (sem_init(sem, 0, value) != 0) {
        std::cout << "Error initializing sem\n";
    }
}
