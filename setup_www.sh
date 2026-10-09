#!/bin/bash
# Creates the test web content that config/webserv_test.conf points at.
# Run this once before launching the server with that config, on any machine.
#   ./setup_www.sh && ./webserv config/webserv_test.conf

set -e
BASE=/tmp/webserv

echo "Setting up test content under $BASE ..."

mkdir -p "$BASE/example/errors" \
         "$BASE/example/images" \
         "$BASE/example/private" \
         "$BASE/example/delete" \
         "$BASE/cgi" \
         "$BASE/uploads" \
         "$BASE/test"

# --- index page for the example server ---
cat > "$BASE/example/index.html" << 'HTML'
<!DOCTYPE html>
<html><head><title>webserv</title></head>
<body><h1>It works!</h1><p>Static file served by webserv.</p></body></html>
HTML

# --- custom error page (config: error_page 404 /errors/404.html) ---
cat > "$BASE/example/errors/404.html" << 'HTML'
<html><body><h1>404 - Custom Not Found</h1></body></html>
HTML

# --- a file to see in the autoindex of /images ---
echo "sample image placeholder" > "$BASE/example/images/readme.txt"

# --- a file for DELETE testing ---
echo "delete me" > "$BASE/example/delete/target.txt"

# --- a CGI script (config: location /cgi, cgi_handler .py /usr/bin/python3) ---
cat > "$BASE/cgi/hello.py" << 'PY'
#!/usr/bin/env python3
import os, sys
body = sys.stdin.read()
print("Content-Type: text/plain")
print()
print("Hello from CGI!")
print("METHOD =", os.environ.get("REQUEST_METHOD", "?"))
print("QUERY  =", os.environ.get("QUERY_STRING", "?"))
print("BODY   =", repr(body))
PY
chmod +x "$BASE/cgi/hello.py"

# --- index for the second (test.com) server block ---
echo '<h1>test.com server</h1>' > "$BASE/test/index.html"

echo "Done. Now run:  ./webserv config/webserv_test.conf"