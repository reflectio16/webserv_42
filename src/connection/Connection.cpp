#include "Connection.hpp"

// Constructor: sane defaults for a freshly accepted client.
// NOTE: the initializer list is in DECLARATION ORDER on purpose -- with
// -Wall -Wextra -Werror, listing members out of order triggers -Wreorder,
// which becomes a hard error. inbuf/outbuf/cgiStdin/cgiBuf/parser are omitted
// because they default-construct empty on their own.
Connection::Connection(int clientFd)
    : fd(clientFd),
      state(READING_REQUEST),
      parsePos(0),
      writeOffset(0),
      keepAlive(true),          // HTTP/1.1 default; the parser may flip it
      lastActivityMs(0),
      cgiPid(-1),
      cgiStdinFd(-1),
      cgiStdoutFd(-1),
      cgiStdinOffset(0),
      cgiStartMs(0)
{}

// ---- read side --------------------------------------------------------------

// Bytes sitting in the tape that the parser hasn't consumed yet. Use this
// (not inbuf.size()) for the max-request guard, so old consumed bytes from
// earlier keep-alive requests don't false-trip the limit.
std::size_t Connection::unconsumedBytes() const {
    return inbuf.size() - parsePos;
}

// Is a whole (pipelined) request already sitting past the cursor? After
// finishing a response on keep-alive, check this BEFORE waiting on POLLIN --
// if true, run the parser immediately instead of blocking for bytes that
// already arrived.
bool Connection::hasBufferedRequest() const {
    return parsePos < inbuf.size();
}

// Drop the consumed prefix and rebase the cursor. ONLY safe at a request
// boundary (parsePos points at clean, fully-consumed bytes). Without this,
// a long-lived keep-alive connection's inbuf grows without bound.
void Connection::compact() {
    if (parsePos > 0) {
        inbuf.erase(0, parsePos);
        parsePos = 0;
    }
}

// ---- write side -------------------------------------------------------------

// Arm the write phase: hand over the response bytes and switch state. Bundles
// the writeOffset reset so it can't be forgotten (forgetting it re-sends the
// tail of a previous response).
void Connection::queueResponse(const std::string& bytes) {
    outbuf      = bytes;
    writeOffset = 0;
    state       = WRITING_RESPONSE;
}

bool Connection::responseFullySent() const {
    return writeOffset >= outbuf.size();
}

// ---- keep-alive transition --------------------------------------------------

// WRITING_RESPONSE -> READING_REQUEST. Resets everything belonging to the
// finished request while preserving any pipelined leftover bytes. Missing any
// one of these resets is the classic "second request behaves weirdly" bug.
// The caller must ALSO flip the poll mask back to POLLIN-only.
void Connection::resetForNextRequest() {
    //parser.reset();     // clear partial-parse state
    outbuf.clear();     // discard the sent response
    writeOffset = 0;
    compact();          // drop consumed bytes, keep the remainder
    state = READING_REQUEST;
}
