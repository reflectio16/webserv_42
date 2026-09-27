#ifndef OUTCOME_HPP
#define OUTCOME_HPP

#include <string>
#include <vector>

// Shared type at the ResponseBuilder <-> loop/CgiProcess seam.
// build() returns one of these: either a finished HTTP response to send, or a
// CGI recipe for the loop to run. Neither side "owns" it -- it lives here so
// both include the same definition. (See the ResponseBuilder contract.)
struct Outcome {
    enum Kind { RESPONSE, CGI };

    Kind        kind;
    bool        keepAlive;         // the loop copies this onto conn.keepAlive

    // ---- kind == RESPONSE ----
    std::string responseBytes;     // complete HTTP/1.1 response, ready for outbuf

    // ---- kind == CGI ----  (the recipe the loop's CgiProcess runs)
    std::string              cgiInterpreter; // argv[0], e.g. "/usr/bin/python3" ("" if directly executable)
    std::string              cgiScriptPath;  // filesystem path to the script
    std::vector<std::string> cgiEnv;         // "KEY=VALUE" strings for execve
    std::string              cgiBody;        // request body to feed the script's stdin

    Outcome() : kind(RESPONSE), keepAlive(true) {}
};

#endif // OUTCOME_HPP