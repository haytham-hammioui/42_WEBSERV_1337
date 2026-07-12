#!/usr/bin/env python3
import os
import datetime

print("Content-Type: text/html\r")
print("\r")
print("<html><body>")
print("<h1>Python CGI Test</h1>")
print("<p>Time: " + str(datetime.datetime.now()) + "</p>")
print("<p>Method: " + os.environ.get('REQUEST_METHOD', 'unknown') + "</p>")
print("</body></html>")