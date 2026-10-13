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
- Methods: GET, POST (including multipart file upload), DELETE
- HTTP redirects
- CGI execution with per-request environment and a runaway-script timeout
- Per-location configuration (root, index, methods, client body size limit, etc.)

## Instructions

**Requirements:** a C++98 compiler and `make` (Linux).

**Build & run:**

```bash
make
./webserv <config_file>
```

Targets: `make clean`, `make fclean`, `make re`.

Compilation flags:

```text
-Wall -Wextra -Werror -std=c++98
```

### Quick start with the demo environment

The repository contains a small browser-based evaluation environment.

First, create the runtime test tree under `/tmp/webserv-demo-simple`:

```bash
chmod +x scripts/install_demo.sh
./scripts/install_demo.sh
```

Then start the server with:

```bash
./webserv config/demo.conf
```

Open the demo website in a browser:

```text
http://127.0.0.1:8080/
```

The demo contains simple pages for GET, POST, DELETE, CGI, redirects, uploads,
autoindex and common HTTP errors. The browser is only used to trigger requests;
the complete HTTP responses can be inspected directly in the terminal running
`webserv`.

For the virtual-host demonstration, you can optionally add local hostnames:

```bash
echo '127.0.0.1 example.com test.com' | sudo tee -a /etc/hosts
```

Then use:

```text
http://example.com:8080/
http://test.com:8080/
```

The second URL is served by a different `server {}` block on the same listening
endpoint.

### Quick checks

Use `127.0.0.1`, not `localhost`:

```bash
curl -v http://127.0.0.1:8080/
curl -v http://127.0.0.1:8080/cgi/hello.py
curl -v http://127.0.0.1:8080/nope
```

> **Note:** the server listens on **IPv4 only**. Test with `127.0.0.1`, not
> `localhost`, since `localhost` may resolve to the IPv6 address `::1`, which the
> server does not listen on. Some tools, such as `siege`, do not fall back to IPv4.

### Stress / leak testing

```bash
siege -b -c 25 -t 20S http://127.0.0.1:8080/
siege -b -c 25 -t 20S http://127.0.0.1:8080/cgi/hello.py
```

Availability should stay near 100%.

After a run, the idle open-fd count:

```bash
ls /proc/<pid>/fd | wc -l
```

should return to its baseline, with no file-descriptor leak and no `<defunct>`
processes left behind.

## Architecture & Design Choices

The project is split into two layers with a deliberately clean boundary:

- an **I/O layer** — the event loop, sockets, connections, and CGI process
  management (`src/core`, `src/socket`, `src/connection`, `src/cgi`);
- an **HTTP layer** — request parsing, configuration, and response building
  (`src/http`, `src/config`).

The two communicate only through small, frozen interfaces, which allowed the two
authors to build and test their halves independently and integrate them as clean
drop-ins rather than one tightly coupled implementation.

### Why a single-threaded, non-blocking event loop

A web server is **I/O-bound**: at any instant most connections are not computing
anything — they are waiting for the network, for example a slow client sending a
request in fragments or a client whose receive buffer is momentarily full.

The naive alternative, one thread per connection that blocks on `read` / `write`,
pays for that waiting with a thread and its stack per client and scales poorly when
the number of simultaneous connections grows.

Instead, a **single thread** runs one `poll()` loop that asks the kernel *which*
file descriptors are ready and acts only on those. One slow or malicious client
does not stall the others, there are **no locks and no data races**, and the server
can handle many concurrent clients cheaply.

This is the same readiness-based model used by servers such as nginx and by other
event-driven systems. Everything else in the design follows from committing to it.

### The `Connection` object: explicit state in place of a call stack

Because the loop returns to `poll()` after every small step, there is no call stack
holding "where we are" in handling a given client between wakeups.

Each client's progress therefore lives explicitly in a `Connection` object:
its input buffer, parse cursor, output buffer and write offset, keep-alive state
and — while a CGI runs — the child pid and pipe file descriptors.

The `Connection` is effectively the per-client saved state that a dedicated
thread's stack would otherwise carry.

### Buffering: an append-only tape + a moving cursor

Data arrives in fragments across many `poll()` wakeups, so reads and writes must
be partial-safe.

The input buffer is treated as an **append-only tape**: each `recv()` appends new
bytes, while a **parse cursor** marks how far the parser has consumed.

This makes keep-alive and request **pipelining** natural: a second request that
arrived in the same read can already be sitting after the first request in the
buffer. At request boundaries, the consumed prefix can be compacted away.

The output side mirrors this design with a write offset, so a response that cannot
be sent in one `send()` simply resumes on the next writable `poll()` event.

### The non-blocking discipline (and the 42 constraints behind it)

- Every socket and pipe is `O_NONBLOCK`; I/O happens only after `poll()` reports
  readiness, never speculatively.
- One `recv()` / `send()` is performed per readiness event, and the return value
  determines the outcome.
- `SIGPIPE` is ignored, so a peer disconnecting during a write cannot terminate
  the server.
- `FD_CLOEXEC` is set on listening and client sockets so CGI children do not
  inherit and leak them.

### CGI: a subprocess folded into the same loop

A CGI script can run for an arbitrary amount of time, so it cannot be executed
synchronously without freezing every other client.

Instead the server **forks the CGI process and inserts its pipes into the same
`poll()` loop**, treating them as two additional file descriptors with the roles
`CGI_STDIN` and `CGI_STDOUT`.

In the child process:

- the pipes are connected to stdin / stdout with `dup2()`;
- the process changes directory to the script directory;
- `execve()` launches the configured interpreter;
- `_exit()` is used on failure so a failed `execve()` never falls back into the
  server code.

In the parent process:

- the request body is streamed to the child's stdin;
- the write end of the pipe is closed to signal EOF;
- the child's stdout is drained incrementally;
- on EOF, the child is reaped with `waitpid()`;
- the CGI output is converted into a valid HTTP response.

CGI state lives on the owning `Connection`, so closing a connection also tears
down any in-flight CGI: child termination, reaping and pipe cleanup all remain
bound to that client.

A runaway-script timeout kills and reaps CGI processes that run for too long and
returns `504 Gateway Timeout`.

### Decoupling that protects the design

`CgiProcess` never modifies the event loop's poll set directly. It operates on the
`Connection` and exposes state to `Server`, while `Server` remains the single owner
of the poll set.

Keeping that ownership in one place, and keeping dependencies pointing in one
direction (`Server` → `CgiProcess`), makes the event loop easier to reason about
and keeps CGI management testable in isolation.

### HTTP request parsing

The HTTP parser is incremental: it never assumes that one `recv()` contains one
complete request.

A request may arrive as:

```text
GET / HTTP/1.1\r\n
Host: example.com\r\n
\r\n
```

or be split across several TCP reads at any arbitrary byte boundary. The parser
therefore keeps its own parsing state and resumes exactly where it stopped when
new bytes are appended to the connection buffer.

The parsing pipeline is split into distinct stages:

1. request line;
2. headers;
3. body framing;
4. body parsing;
5. completed request.

The request line validates the method, request-target and HTTP version. Headers are
parsed separately and normalized enough for later semantic checks.

For HTTP/1.1, the parser enforces the presence of `Host` and rejects malformed or
ambiguous framing such as an invalid `Content-Length` or a request containing both
`Content-Length` and `Transfer-Encoding: chunked`.

Both supported body representations are handled incrementally:

- `Content-Length`: the parser waits until exactly the announced number of bytes
  has arrived;
- `Transfer-Encoding: chunked`: chunk sizes, data, the terminating zero-sized
  chunk and trailers are parsed as a state machine.

Chunked request bodies are **dechunked before being exposed to the rest of the
server**. This is particularly important for CGI, which receives a normal request
body instead of HTTP chunk framing.

The parser also tracks how many bytes were consumed. This allows the server to
preserve unconsumed bytes already received after the current request, which is
what makes HTTP keep-alive and pipelining work without reparsing or losing data.

### Configuration file & parsing

The configuration parser implements a deliberately small nginx-like grammar.

A configuration is made of one or more `server {}` blocks containing server-level
directives and nested `location` blocks.

Supported directives include:

```text
listen
server_name
root
client_max_body_size
error_page

location
methods
index
autoindex
upload_dir
redirect
cgi_handler
```

The parser is split into separate responsibilities rather than processing the file
with one large function.

First, the configuration file is read and tokenized. Structural tokens such as
`{`, `}` and `;` are preserved, which allows the parser to reason about nested
blocks independently from whitespace and line layout.

The parser then builds strongly typed objects:

```text
Config
└── ServerBlock
    └── LocationBlock
```

Parsing and validation are intentionally separate. Syntax is parsed first, then
semantic validation rejects invalid states such as duplicate directives, invalid
ports, unsupported methods, duplicate locations, invalid `autoindex` values,
invalid body-size values, malformed error-page declarations or an `upload_dir`
configured where POST is not allowed.

This separation keeps parsing deterministic and avoids spreading configuration
rules into the HTTP response layer.

The resulting `Config` object is read-only from the point of view of request
handling: once startup succeeds, the server can query endpoints, virtual hosts and
location settings without reparsing the configuration.

### Response building

`ResponseBuilder` is the HTTP-side dispatcher.

Given a parsed `HttpRequest` and the listening endpoint that received it, it first
selects the appropriate `server {}` block using the request's `Host` header and
then resolves the matching `location`.

Location matching uses the longest valid prefix while respecting path boundaries,
so a location such as:

```text
/images
```

matches:

```text
/images/file.txt
```

but not:

```text
/imagesfoo
```

The request path is normalized before routing. Once the location has been chosen,
the URI is mapped to the filesystem according to that location's `root`.

From there, the builder dispatches according to configuration and method:

- **GET** serves static files;
- directories use the configured `index`, or generate an autoindex when enabled;
- **POST** can write raw request bodies or process multipart file uploads;
- **DELETE** removes the selected filesystem resource;
- redirects return their configured status and `Location` header;
- disallowed methods return `405 Method Not Allowed`;
- unsupported methods return the appropriate error response;
- configured custom error pages are used when available.

The response layer is also responsible for body-size enforcement. Requests larger
than the configured `client_max_body_size` are rejected before an upload or CGI is
started.

CGI routing remains deliberately separate from process management.

When the selected resource is configured as CGI, `ResponseBuilder` does **not**
`fork()` or execute anything itself. Instead it creates an `Outcome` describing
what the I/O layer must launch:

```text
interpreter
script path
environment
request body
keep-alive decision
```

`Server` then passes this recipe to `CgiProcess`.

When CGI finishes, the HTTP layer receives the raw CGI stdout, parses its headers
and body, and converts it into the final HTTP/1.1 response.

This boundary keeps the HTTP layer independent from `poll()`, pipes and child
process lifetime management while keeping the I/O layer independent from routing
and HTTP semantics.

## Project Structure

```text
cgi/             CGI scripts used by the browser demo
config/          server configuration files, including demo.conf
scripts/         demo environment setup scripts
test/            content for the secondary virtual host
www/             browser demo: HTML, CSS, JavaScript, images and error pages

src/core/        Server           event loop, dispatch, accept, read/write        [meelma]
src/socket/      ListeningSocket  non-blocking listening sockets (getaddrinfo)    [meelma]
src/connection/  Connection       per-client state + helpers                      [meelma]
src/cgi/         CgiProcess       fork/exec/pipe, stdin/stdout, cleanup            [meelma]

src/http/        RequestParser, HttpRequest, Outcome, ResponseBuilder              [fmoulin]
src/config/      Config, ConfigParser, ServerBlock, LocationBlock                  [fmoulin]

main.cpp         entry point
Makefile
README.md
```

The top-level `www/`, `cgi/` and `test/` directories are source material for the
evaluation environment. `scripts/install_demo.sh` copies them into an isolated
runtime tree under:

```text
/tmp/webserv-demo-simple
```

so the browser demo can be reset without modifying the repository files.

## Resources

- **RFC 7230 / 7231** — HTTP/1.1 message syntax, routing, and semantics
- **RFC 3875** — The Common Gateway Interface (CGI/1.1)
- Beej's Guide to Network Programming — sockets, `poll()`, non-blocking I/O
- `man` pages: `poll(2)`, `socket(2)`, `bind(2)`, `accept(2)`, `send(2)`,
  `recv(2)`, `fork(2)`, `execve(2)`, `pipe(2)`, `dup2(2)`, `waitpid(2)`,
  `fcntl(2)`
- nginx documentation — behavioral reference for configuration and features

### Use of AI

An LLM assistant was used during this project in the following ways:

- **Conceptual understanding** — explaining the systems concepts behind the
  implementation (non-blocking I/O, the `poll()` event loop, the connection state
  machine, `fork` / `execve` / `pipe`, CGI in an event loop, zombie reaping) so the
  code was written with full understanding rather than copied.
- **Design discussion** — reasoning through architectural decisions and their
  trade-offs (where state should live, decoupling the I/O and HTTP layers,
  freezing the interfaces between the two authors' work).
- **Debugging and code review** — interpreting compiler/linker errors, reviewing
  functions line by line, and diagnosing issues found during stress testing.
- **Testing guidance** — building manual and automated tests for routing, request
  parsing, uploads, CGI, timeouts, keep-alive, pipelining, concurrency,
  file-descriptor leaks and zombie-process leaks.
- **Documentation support** — helping structure and clarify the project README and
  evaluation/demo environment.

All code was written, understood, integrated, and tested by the authors.
