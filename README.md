*This project has been created as part of the 42 curriculum by meelma, fmoulin.*

# webserv

## Description

**webserv** is a non-blocking HTTP/1.1 web server written from scratch in C++98.
The goal is to build a server, similar in spirit to nginx, that serves static
websites, runs dynamic content through CGI, and handles many clients at once — all
on a **single thread** using one I/O-multiplexing (`poll()`) event loop, without
ever blocking on a client.

The server reads an nginx-like configuration file (ports, roots, locations, allowed
methods, error pages, redirects, uploads, CGI handlers), parses incoming HTTP
requests, serves files or executes CGI scripts, and returns well-formed responses
while staying responsive under heavy concurrent load.

**Key features**
- Single-threaded, non-blocking, event-driven (`poll()`)
- HTTP/1.1 with keep-alive and request pipelining
- Multiple servers / ports and virtual hosts
- Static file serving, autoindex, custom error pages
- Methods: GET, POST (incl. multipart file upload), DELETE
- HTTP redirects
- CGI execution with per-request environment and a runaway-script timeout
- Per-location configuration (root, index, methods, client body size limit, etc.)

## Instructions

**Requirements:** a C++98 compiler and `make` (Linux).

**Build & run:**
```bash
make                                   # produces ./webserv
./webserv <config_file>
```
Targets: `make clean`, `make fclean`, `make re`. Flags: `-Wall -Wextra -Werror -std=c++98`.

**Quick start with the sample config:**
```bash
./setup_www.sh                         # creates the test content under /tmp/webserv
./webserv config/webserv_test.conf
```

**Quick checks (use 127.0.0.1, not localhost):**
```bash
curl -v http://127.0.0.1:8080/              # static file
curl -v http://127.0.0.1:8080/cgi/hello.py  # CGI
curl -v http://127.0.0.1:8080/nope          # 404
```

> **Note:** the server listens on **IPv4 only**. Test with `127.0.0.1`, not
> `localhost`, since `localhost` may resolve to the IPv6 address `::1`, which the
> server does not listen on (some tools, e.g. `siege`, won't fall back to IPv4).

**Stress / leak testing:**
```bash
siege -b -c 25 -t 20S http://127.0.0.1:8080/
siege -b -c 25 -t 20S http://127.0.0.1:8080/cgi/hello.py
```
Availability should stay near 100%; the idle open-fd count
(`ls /proc/<pid>/fd | wc -l`) should return to baseline after a run (no fd leak),
with no `<defunct>` processes (no zombie leak).

## Architecture & Design Choices

The project is split into two layers with a deliberately clean boundary:

- an **I/O layer** — the event loop, sockets, connections, and CGI process
  management (`src/core`, `src/socket`, `src/connection`, `src/cgi`);
- an **HTTP layer** — request parsing, configuration, and response building
  (`src/http`, `src/config`).

The two communicate only through small, frozen interfaces, which let the two
authors build and test their halves independently and integrate them as clean
drop-ins rather than a merge.

### Why a single-threaded, non-blocking event loop

A web server is **I/O-bound**: at any instant most connections are not computing
anything — they are waiting for the network (a slow client sending a request in
fragments, a client whose receive buffer is momentarily full). The naive
alternative, one thread per connection that blocks on `read`/`write`, pays for
that waiting with a thread (and its stack) per client and collapses under
thousands of connections.

Instead, a **single thread** runs one `poll()` loop that asks the kernel *which*
file descriptors are ready and acts only on those. One slow or malicious client
can never stall the others, there are **no locks and no data races**, and the
server scales to many concurrent clients cheaply. This is the same
readiness-based model used by nginx, Redis, and Node. Everything else in the
design follows from committing to it.

### The `Connection` object: explicit state in place of a call stack

Because the loop returns to `poll()` after every small step, there is no call
stack holding "where we are" in handling a given client between wakeups. So each
client's progress lives explicitly on the heap in a **`Connection`** object:
its input buffer, its parse cursor, its output buffer and write offset, its
keep-alive flag, and — while a CGI runs — the child's pid and pipe fds. The
`Connection` *is* the per-client saved state that a thread's stack would
otherwise carry.

### Buffering: an append-only tape + a moving cursor

Data arrives in fragments across many `poll()` wakeups, so reads and writes must
be partial-safe. The input buffer is treated as an **append-only tape**: each
`recv` appends to it, and a **parse cursor** marks how far the parser has
consumed. This makes keep-alive and request **pipelining** almost free — a second
request that arrived in the same read is already sitting past the cursor — at the
cost of a compaction step at request boundaries to reclaim memory. The output
side mirrors this with a write offset, so a response too large for one `send`
simply resumes on the next writable wakeup.

### The non-blocking discipline (and the 42 constraints behind it)

- Every socket and pipe is `O_NONBLOCK`; **I/O happens only after `poll()` reports
  readiness**, never speculatively.
- **One `recv`/`send` per readiness event**, and the return value alone decides
  the outcome (0 = peer closed, <0 = gone). `errno` is never inspected after I/O —
  which is both the project rule and what the readiness-based design makes
  unnecessary.
- `SIGPIPE` is ignored, so a peer that disconnects mid-write cannot kill the
  server.
- `FD_CLOEXEC` is set on listening and client sockets so CGI children do not
  inherit (and leak) them.

### CGI: a subprocess folded into the same loop

CGI is the systems-heavy core of this half. A CGI script can run for an arbitrary
time, so it cannot be run synchronously without freezing every other client.
Instead the server **forks the script and splices its two pipes into the same
`poll()` loop**, treating them as two more file descriptors with roles
`CGI_STDIN` / `CGI_STDOUT`:

- In the child: `dup2` the pipes onto stdin/stdout, `chdir` into the script's
  directory (child only — `chdir` is process-wide), then `execve` the interpreter;
  `_exit` on failure so a failed exec never falls back into the server.
- In the parent: the request body is streamed to the child's stdin and the write
  end is **closed to signal EOF** (otherwise a script that reads to EOF hangs
  forever); the child's stdout is drained into a buffer, and on EOF the child is
  reaped with `waitpid` (no zombies) and its raw output is turned into a valid
  HTTP response.

**CGI state lives on the owning `Connection`**, so closing a connection
automatically tears down any in-flight CGI (kill + reap + close pipes) — the CGI's
lifetime is a strict subset of the connection's.

A **runaway-script timeout** kills and reaps a CGI that runs too long and returns
`504 Gateway Timeout`. Since no wall-clock function is on the allowed list, elapsed
time is derived from the `poll()` timeout: the clock only advances when `poll`
actually times out (the loop is idle), which keeps it correct under load rather
than firing falsely when the loop is busy.

### Decoupling that protects the design

`CgiProcess` never touches the event loop's poll set directly — it operates on the
`Connection` and signals the `Server` (via flags and return values), and the
`Server` alone owns the poll set. Keeping that single owner of poll-set state, and
keeping dependencies pointing one direction (`Server` → `CgiProcess`, never back),
is what makes the loop straightforward to reason about and the CGI code testable
in isolation.


<!-- TODO François: -- His design parts --> 

### HTTP request parsing
<!-- François: your design reasoning for the request parser -->

### Configuration file & parsing
<!-- François: config format + how you parse/validate it -->

### Response building
<!-- François: static files, routing/locations, methods, autoindex, uploads, redirects -->










## Project Structure

```
src/core/        Server          event loop, dispatch, accept, read/write       [meelma]
src/socket/      ListeningSocket  non-blocking listening sockets (getaddrinfo)   [meelma]
src/connection/  Connection       per-client state + helpers                    [meelma]
src/cgi/         CgiProcess       fork/exec/pipe, feed stdin, drain stdout, reap [meelma]
src/http/        RequestParser, HttpRequest, Outcome, ResponseBuilder           [fmoulin]
src/config/      Config, ConfigParser, ServerBlock, LocationBlock               [fmoulin]
main.cpp         entry point
Makefile
setup_www.sh     creates the /tmp/webserv test content
config/          sample configuration files
```

## Resources

- **RFC 7230 / 7231** — HTTP/1.1 message syntax, routing, and semantics
- **RFC 3875** — The Common Gateway Interface (CGI/1.1)
- Beej's Guide to Network Programming — sockets, `poll`, non-blocking I/O
- `man` pages: `poll(2)`, `socket(2)`, `bind(2)`, `accept(2)`, `send(2)`,
  `recv(2)`, `fork(2)`, `execve(2)`, `pipe(2)`, `dup2(2)`, `waitpid(2)`, `fcntl(2)`
- nginx documentation (behavioral reference for configuration and features)

### Use of AI

An LLM assistant was used during this project in the following ways:
- **Conceptual understanding** — explaining the systems concepts behind the
  implementation (non-blocking I/O, the `poll()` event loop, the connection state
  machine, `fork`/`execve`/`pipe`, CGI in an event loop, zombie reaping) so the
  code was written with full understanding rather than copied.
- **Design discussion** — reasoning through architectural decisions and their
  trade-offs (where state should live, decoupling the I/O and HTTP layers,
  freezing the interfaces between the two authors' work).
- **Debugging and code review** — interpreting compiler/linker errors, reviewing
  functions line by line, and diagnosing issues found during stress testing
  (e.g. a timeout clock that misbehaved under load).
- **Testing guidance** — setting up and interpreting `siege` stress tests and
  file-descriptor / zombie leak checks.

All code was written, understood, integrated, and tested by the authors.