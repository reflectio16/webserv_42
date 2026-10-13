#!/usr/bin/env python3
import os
import sys

body = sys.stdin.read()
print("Content-Type: text/plain")
print()
for key in ["REQUEST_METHOD","QUERY_STRING","CONTENT_LENGTH","CONTENT_TYPE","SERVER_NAME","SERVER_PORT","SCRIPT_NAME"]:
    print("%s = %s" % (key, os.environ.get(key, "?")))
print("BODY =", repr(body))
