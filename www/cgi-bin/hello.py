#!/usr/bin/env python3
import os, sys
body_in = sys.stdin.read()
print("Content-Type: text/plain")
print()
print("Hello from a CGI script!")
print("REQUEST_METHOD =", os.environ.get("REQUEST_METHOD", "(unset)"))
print("QUERY_STRING   =", os.environ.get("QUERY_STRING", "(unset)"))
print("Body I received:", repr(body_in))
