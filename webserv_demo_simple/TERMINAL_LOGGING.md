# Print complete HTTP responses in the webserv terminal

The website intentionally does **not** display server responses.

To print every complete response in the terminal, the cleanest place in the current architecture is `Connection::queueResponse()`, because normal responses, CGI responses, CGI errors and timeouts all eventually pass through it.

In `src/connection/Connection.cpp`, add:

```cpp
#include <iostream>
```

Then change:

```cpp
void Connection::queueResponse(const std::string& bytes) {
    outbuf      = bytes;
    writeOffset = 0;
    state       = WRITING_RESPONSE;
}
```

to:

```cpp
void Connection::queueResponse(const std::string& bytes) {
    std::cout << "\n";
    std::cout << "================ HTTP RESPONSE ================\n";
    std::cout << bytes;
    if (bytes.empty() || bytes[bytes.size() - 1] != '\n')
        std::cout << "\n";
    std::cout << "===============================================\n";
    std::cout << std::endl;

    outbuf      = bytes;
    writeOffset = 0;
    state       = WRITING_RESPONSE;
}
```

With the demo website, the responses are text/HTML/plain text, so the complete body is readable in the terminal.

Example:

```text
================ HTTP RESPONSE ================
HTTP/1.1 201 Created
Content-Type: text/plain
Content-Length: 0
Connection: keep-alive

===============================================
```

For a CGI:

```text
================ HTTP RESPONSE ================
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 63
Connection: keep-alive

Hello from CGI!
METHOD = POST
QUERY  = x=1
BODY   = 'name=Bob'
===============================================
```

If you also want the complete parsed REQUEST printed immediately before each response, add a request logger separately in `Server::processInput()` once `HttpRequest req` has been produced. Do not try to make the browser responsible for terminal logging: the terminal output belongs to the C++ server.
