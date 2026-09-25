#include "Server.hpp"
#include <iostream>

static void	testRequest(
	const std::string &name,
	const std::string &raw,
	RequestParser::Status expected)
{
	RequestParser parser;

	RequestParser::Status result = parser.parse(raw, 0);

	std::cout << name << " : ";

	if (result == expected)
		std::cout << "OK";
	else
		std::cout << "FAIL"
				  << " expected=" << expected
				  << " got=" << result;

	std::cout << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <config file>" << std::endl;
        return 1;
    }
    try {
        // Server server(argv[1]);
        // server.run();

		testRequest(
			"valid HTTP/1.1",
			"GET / HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"\r\n",
			RequestParser::COMPLETE
		);

		testRequest(
			"HTTP/1.1 missing Host",
			"GET / HTTP/1.1\r\n"
			"User-Agent: test\r\n"
			"\r\n",
			RequestParser::PARSE_ERROR
		);

		testRequest(
			"empty Host",
			"GET / HTTP/1.1\r\n"
			"Host:     \r\n"
			"\r\n",
			RequestParser::PARSE_ERROR
		);

		testRequest(
			"invalid HTTP version",
			"GET / HTTP/42.0\r\n"
			"Host: example.com\r\n"
			"\r\n",
			RequestParser::PARSE_ERROR
		);

		testRequest(
			"invalid target",
			"GET banana HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"\r\n",
			RequestParser::PARSE_ERROR
		);

		testRequest(
			"header without colon",
			"GET / HTTP/1.1\r\n"
			"Host example.com\r\n"
			"\r\n",
			RequestParser::PARSE_ERROR
		);

		testRequest(
			"duplicate Host",
			"GET / HTTP/1.1\r\n"
			"Host: foo.com\r\n"
			"Host: bar.com\r\n"
			"\r\n",
			RequestParser::PARSE_ERROR
		);

		testRequest(
			"duplicate Content-Length",
			"POST /upload HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Content-Length: 5\r\n"
			"Content-Length: 5\r\n"
			"\r\n"
			"hello",
			RequestParser::PARSE_ERROR
		);

		testRequest(
			"invalid Content-Length",
			"POST /upload HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Content-Length: banana\r\n"
			"\r\n",
			RequestParser::PARSE_ERROR
		);

		testRequest(
			"negative Content-Length",
			"POST /upload HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Content-Length: -5\r\n"
			"\r\n",
			RequestParser::PARSE_ERROR
		);

		testRequest(
			"CL + Transfer-Encoding",
			"POST /upload HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Content-Length: 5\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"hello",
			RequestParser::PARSE_ERROR
		);

		testRequest(
			"unsupported transfer encoding",
			"POST /upload HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Transfer-Encoding: banana\r\n"
			"\r\n",
			RequestParser::PARSE_ERROR
		);

		testRequest(
			"invalid chunk size",
			"POST /upload HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"ZZ\r\n"
			"hello\r\n"
			"0\r\n"
			"\r\n",
			RequestParser::PARSE_ERROR
		);

		testRequest(
			"valid chunked",
			"POST /upload HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"4\r\n"
			"Wiki\r\n"
			"5\r\n"
			"pedia\r\n"
			"0\r\n"
			"\r\n",
			RequestParser::COMPLETE
		);

		testRequest(
			"valid chunk extension",
			"POST /upload HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Transfer-Encoding: chunked\r\n"
			"\r\n"
			"4;foo=bar\r\n"
			"Wiki\r\n"
			"0\r\n"
			"\r\n",
			RequestParser::COMPLETE
		);

		testRequest(
			"unsupported but syntactically valid method",
			"PUT /hello HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"\r\n",
			RequestParser::COMPLETE
		);
    }
    catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
