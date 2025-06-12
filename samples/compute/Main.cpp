#include "compute.h"
#include <iostream>

int main() {
    Compute app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return 0;
}