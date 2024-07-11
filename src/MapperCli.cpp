#include <iostream>

#include "Mapper.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        cout << "Missing filename\nUsage: mapper [INPUT FILE...] [OUTPUT "
                "FILE...]\n";
        return 0;
    }

    executeFile(argv[1], argv[2]);
}
