// Test helper launched by the Windows launcher tests.
// Arguments run in order: print <text> | exit <code> | sleep <milliseconds>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; i += 2) {
        const std::string action = argv[i];
        if (action == "print") std::cout << argv[i + 1] << std::endl;
        else if (action == "exit") return std::atoi(argv[i + 1]);
        else if (action == "sleep") std::this_thread::sleep_for(std::chrono::milliseconds(std::atoi(argv[i + 1])));
        else return 99;
    }
    return 0;
}
