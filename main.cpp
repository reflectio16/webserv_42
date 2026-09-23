#include "Server.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <config file>" << std::endl;
        return 1;
    }
    try {
        Server server(argv[1]);
        server.run();
    }
    catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
