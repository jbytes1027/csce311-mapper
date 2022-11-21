using namespace std;
#include <iostream>

#include "Mapper.h"

int main(int argc, char** argv) {
    if (argc == 3) {
        executeFile(argv[1], argv[2]);
    } else {
        cout << "Missing filename\nUsage: mapper [INPUT FILE...] [OUTPUT "
                "FILE...]\n";
    }
}
