#!/usr/bin/env python3
import os
import sys
import time
time.sleep(8)

# Read the request body from stdin (the server feeds it through the pipe).
body_in = sys.stdin.read()

# CGI output format: header block, blank line, then body.
# NOT valid HTTP yet -- the server's finalizeCgi() wraps this into a response.
print("Content-Type: text/plain")
print()                                    # the blank line is MANDATORY
print("Hello from a CGI script!")
print("REQUEST_METHOD =", os.environ.get("REQUEST_METHOD", "(unset)"))
print("QUERY_STRING   =", os.environ.get("QUERY_STRING", "(unset)"))
print("Body I received:", repr(body_in))