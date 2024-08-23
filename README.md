# Mapper

Mapper is program to concurrently execute map operations read from a file on a concurrent hash map. It was written for an OS class assignment that tested implementing a bounded-buffer and a concurrent data structure using only concurrency primitives.

## Assignment

The basic assignment requirements were as follows:

- Must correctly execute map operations read from a file
- Must be thread safe
- Multi-threaded output must be the same as single-threaded output
- Performance must increase roughly proportional to threads used (up to the number of cores)
- Hash map must be able to hold up to 2^22 items
- Hash map does not have to support rehashing
- Must write tests

## Approach

The core of the program is split into two parts `Mapper::executeStream` and `Mapper::consumeLineThread`. `executeStream` takes in the stream of input, initializes everything, creates the consumer threads, waits for them to finish, and returns the resulting output stream. The consumer, `executeStream`, reads a line, parses it, executes it, and adds the result to the output buffer. Semaphores are used to lock reading, executing, and adding to the output buffer. To keep track of the thread's execution and output turn, the current line number is recorded when reading the line. This is used to determine when its a consumers turn to execute and output. The execution is kept ordered by locking until the map locks internally. This is done by passing a semaphore to the map's operation call. Internally the map uses bucket locking to scale. The consumer uses multiple stages and locks because it was found to be faster. When there are no lines left to read a consumer exits. The main thread knows when the consumers are done by checking the value of a semaphore that keeps track of how many consumers have finished. _See `Mapper.cpp` for details and additional comments._

## Testing Approach

For testing, the map's correctness and scaling are tested. Speed is not tested because it varies from local machine to lab machine and background programs. The correctness of the map operations without threading is tested to ensure the basic logic is sound. For all the multi threaded tests the multi-threaded execution result is compared to a single threaded execution result to ensure output is correct and ordered. By repeatedly inserting, deleting, and looking up the same key with a large number of threads, mapper is stress tested to ensure there are no locking or ordering error. Scaling for mapper is tested by generating input with random keys, timing the execution length for one, two, three, and for consumers and comparing the ratio between one consumer and x consumers. The map is also tested for scaling in the same manner except that a delay is added to every map operations so that the operation time outweighs the overhead from ordering the input and output. _See `MapTest.cpp` for details and additional comments._

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

To compile create and enter a build directory:

    mkdir build && cd build

To create a makefile for building, from the build directory run:

    cmake ..

To build, run:

    make

To test, run:

    ./mapper-test

To run, use:

    ./mapper [INPUT FILE...] [OUTPUT FILE...]
