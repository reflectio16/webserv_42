# Simple webserv browser demo

This version is intentionally much simpler than the previous dashboard.

There is:
- no "latest response" panel;
- no response body displayed by JavaScript;
- no automated dashboard;
- just separate GET / POST / DELETE / CGI / Errors pages and simple buttons/forms.

The intended evaluation workflow is:

```text
browser click
    ↓
real HTTP request
    ↓
webserv handles it
    ↓
COMPLETE HTTP response appears in the webserv terminal
```

## Install the demo tree

```bash
chmod +x scripts/install_demo.sh
./scripts/install_demo.sh
```

## Run webserv

```bash
./webserv /absolute/path/to/webserv_demo_simple/config/demo.conf
```

Open:

```text
http://127.0.0.1:8080/
```

For the virtual-host button, optionally add:

```bash
echo '127.0.0.1 example.com test.com' | sudo tee -a /etc/hosts
```

and open:

```text
http://example.com:8080/
```

## Important: terminal output

Read `TERMINAL_LOGGING.md`.

The browser cannot make your C++ program print anything by itself. The server must log the response bytes it is already about to send. In the current architecture, `Connection::queueResponse()` is the ideal central point to print the complete HTTP response once.
