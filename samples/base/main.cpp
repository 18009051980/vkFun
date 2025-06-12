#include "Base.h"
#include <iostream>

int main() {
    Base app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return 0;
}