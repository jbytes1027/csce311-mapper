# Mapper

Mapper is a program to concurrently execute map operations read from a file on a concurrent hash map. It was written for an OS class assignment that tested implementing a bounded-buffer and a concurrent data structure using only concurrency primitives.

## Assignment

The basic assignment requirements were as follows:

- Must correctly execute map operations read from a file
- Must be thread safe
- Multi-threaded output must be the same as single-threaded output
- Performance must increase roughly proportional to the number of threads used (up to the number of cores)
- The hash map must be able to hold up to 2^22 items
- The hash map does not have to support rehashing
- Must write tests

## Approach

The program reads an input, loads a file into memory, starts several consumers threads, waits for all the file's instructions to be executed, and writes the resulting buffer to a file.

A consumer thread is an endless loop that keeps executing instructions until there are none left. The loop has four stages:

1. Read the next instruction line from the instructions file
2. Parse the instruction from the line
3. Execute the instruction on a concurrent hash map
4. Write the result to a shared buffer

Locked tasks are broken up into small segments to improve concurrency scaling. For instance, the reading, executing, and writing segments lock separately. Also the hash map uses bucket locking to ensure only the segment an operation is accessing is locked.

Additionally, locks are held for as short a time as possible to improve concurrency scaling. Steps like parsing and memory allocation were moved out of locked sections, which resulted in greatly increased concurrency.

To ensure ordering of operations, each consumer records the line number of the instruction line it read. This number is used to ensure the execute and write operations are carried out at the right time.

## Performance and Scaling

Performance is very high and map scaling is very height, however, mapper scaling is not. To see the performance and scaling run the tests, the execution times across one to four cores are printed to the terminal.

    Mapper scaling:
    Executed 4194304 operations with 1 consumer  in 2689ms
    Executed 4194304 operations with 2 consumers in 2056ms
    Executed 4194304 operations with 3 consumers in 2013ms
    Executed 4194304 operations with 4 consumers in 4275ms

    Map scaling:
    Executed 10000 operations with 1 consumer  in 656ms
    Executed 10000 operations with 2 consumers in 335ms
    Executed 10000 operations with 3 consumers in 233ms
    Executed 10000 operations with 4 consumers in 182ms

This shows that mapper is ~1.3 times faster with two and three cores, however it goes down to ~0.65 times less slower with 4 cores and only goes up from there. On the other hand the map itself scales almost perfectly with 2 consumers finishing 1.9 times faster than 1, 3 consumers finishing 2.8 times faster, and 4 consumers finishing 3.6 times faster. It is the high performance of the map that ruins the scaling of mapper. The overhead from parsing and ordering takes up a majority of the runtime compared to the map operation itself. This is demonstrated in the map scaling test results where a delay is added to every map operation, thus making the map operations take a majority of the runtime. This is because three out of the four main sections of the consumer must be executed locked, only the map operations and the parsing can be done mostly concurrently. Also there is significant overhead from checking if its a threads turn to grab a lock and execute or output. This is the cause of the 4+ consumers ruining scaling for mapper as more consumers means more time spent looking for the right thread to execute or output. Among many approaches, locking in phases was tried where when one phase finishes the next lock is grabbed before the current one is released, however this lead to significantly worse performance.

## Compiling & Running

To compile, create and enter a build directory:

    mkdir build && cd build

To create a makefile for building, from the build directory run:

    cmake ..

To build, run:

    make

To test, run:

    ./mapper-test

To run, use:

    ./mapper [INPUT FILE...] [OUTPUT FILE...]
