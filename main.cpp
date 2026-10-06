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
		request.method = "POST";
		request.path = "/upload";
		request.version = "HTTP/1.1";
		request.headers["host"] = "example.com:8080";
		request.headers["content-type"] = "multipart/form-data; boundary=42BOUNDARY";
		request.keepAlive = true;

		request.body =
			"--42BOUNDARY\r\n"
			"Content-Disposition: form-data; "
			"name=\"file\"; filename=\"hello.txt\"\r\n"
			"Content-Type: text/plain\r\n"
			"\r\n"
			"Hello multipart!\n"
			"\r\n--42BOUNDARY--\r\n";
			
		builder.debugRouting(request, endpoint);
    }
    catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
