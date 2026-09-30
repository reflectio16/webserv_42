#include "Server.hpp"
#include <iostream>
#include "src/config/Config.hpp"
#include "src/http/ResponseBuilder.hpp"
#include "src/http/HttpRequest.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <config file>" << std::endl;
        return 1;
    }
    try {
        // Server server(argv[1]);
        // server.run();

		Config	config(argv[1]);

		ResponseBuilder	builder(config);

		Endpoint	endpoint;
		endpoint.host = "0.0.0.0";
		endpoint.port = 8080;

		HttpRequest	request;
		request.method = "GET";
		request.path = "/images";
		request.version = "HTTP/1.1";
		request.headers["host"] = "example.com:8080";
		request.keepAlive = true;

		std::string	host = request.headers["host"];

		builder.debugRouting(request, endpoint);
    }
    catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
