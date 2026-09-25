#include "Server.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <config file>" << std::endl;
        return 1;
    }
    try {
        // Server server(argv[1]);
        // server.run();

		std::string request =
			"POST /upload HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"4\r\n"
			"Wiki\r\n"
			"5\r\n"
			"pedia\r\n"
			"0\r\n"
			"\r\n";

		RequestParser parser;

		RequestParser::Status status =
			parser.parse(request, 0);

		std::cout << "status = " << status << std::endl;
		std::cout << "body = [" << parser.request().body << "]" << std::endl;
		std::cout << "consumed = " << parser.bytesConsumed() << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
