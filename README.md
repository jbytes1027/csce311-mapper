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

The program loads an instruction file into memory, starts several consumer threads, waits for all of the file's instructions to be executed, and writes the result to a file.

A consumer thread is an endless loop that executes instructions until there are none left. The loop has four stages:

1. Read the next instruction line from the instructions file
2. Parse the instruction from the line
3. Execute the instruction on a concurrent hash map
4. Write the result to a shared buffer

Locked tasks are broken into small segments to improve concurrency scaling. For instance, the reading, executing, and writing segments lock separately. Also, the hash map uses bucket locking to ensure only the accessed segment is locked. Moreover, locks are held for as short a time as possible to improve concurrency scaling.

To ensure ordering of operations, each consumer records the line number of the instruction line it reads. When needed, each consumer spins until it's their turn to execute and write.

## Hash Map Scaling

Without the overhead of reading, parsing, and writing results, executing operations on the hash map scales very well. Executing 2^22 insertions with random keys on a 1000-bucket hash map yields the following results:

<p align="center">
  <img src="https://github.com/user-attachments/assets/68b83c0f-ace6-4ff2-8459-adf4e70f6fb4">
</p>

Ideally, the time to execute $x$ operations on $n$ threads in parallel would be:

$$ \frac{\text{Serial execution time of x operations}}{\text{Threads}} $$

However, due to the overhead of locking, the actual measured time was about:

$$ \frac{\text{Serial execution time of x operations}}{\text{Threads}^{0.94}} $$


This formula works up until 9 consumer threads are used, at which point a jump occurs and the rate of decrease becomes linear. This change happens because the last 8 threads were run on virtual cores. The benchmark was run on a machine with 8 physical cores and 16 virtual cores.

When run with 17 threads, the execution time skyrockets up to 17060ms. This is a side effect of using spin-locks for ordering operations.

## Mapper Scaling

For Mapper, the program as a whole, scaling is almost nonexistent:

<p align="center">
  <img src="https://github.com/user-attachments/assets/f9291217-6b3b-493f-a910-82ab6cb9c168">
</p>

Over 95% of the program's runtime is spent on the consumer. With more consumer threads, the performance gain from parallel instruction execution on the hash map is quickly overshadowed by the increased cost of the reading and writing output. The cost of ensuring reading and writing is done serially increases proportionally to the number of threads due to greater lock contention. The leveling out after 8 threads is attributed to using virtual cores.

How the different parts of the consumer scale can be seen by comparing the [flame graph](https://brendangregg.com/flamegraphs.html) of one consumer to the flame graph of eight consumers.

<p align="center">
  <img src="https://miscfiles.blob.core.windows.net/csce311-lab3/one_thread.svg">
</p>
<p align="center">
  <img src="https://miscfiles.blob.core.windows.net/csce311-lab3/eight_threads.svg">
</p>

Note that the large empty space above `consumeLineThread` is from spin locking.

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
