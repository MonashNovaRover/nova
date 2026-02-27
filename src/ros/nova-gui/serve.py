#!/usr/bin/env python3

import http.server
import socketserver
import os
import sys

# This script is used to serve the built gui as a http server and be compatible with how
# vite builds websites.

class CustomHTTPRequestHandler(
    http.server.SimpleHTTPRequestHandler
):
    def translate_path(self, path):
        ret = super().translate_path(path)
        # allow client side routing
        if not os.path.exists(ret):
            return super().translate_path("/")
        return ret

    def send_response_only(self, code, message=None):
        # make sure the gui isn't cached as if you restart the server
        # with a different build you don't want it cached
        super().send_response_only(code, message)
        # https://stackoverflow.com/questions/42341039/remove-cache-in-a-python-http-server#62482117
        self.send_header('Cache-Control', 'no-store, must-revalidate')
        self.send_header('Expires', '0')

if __name__ == '__main__':
    try:
        if len(sys.argv) > 3:
            raise ValueError
        path = sys.argv[1]
        if len(sys.argv) >= 3:
            port = int(sys.argv[2])
            if port < 0 or port > 2**16-1:
                raise ValueError
        else:
            port = 8081 # tileserver is 8080
    except (ValueError, IndexError):
        print(f"Usage: {sys.argv[0]} /path/to/result/share/nova-gui/www [port]")
        exit(1)

    try:
        os.chdir(path)
    except FileNotFoundError:
        print(f"Directory {path} does not exist.")
        exit(1)
    except NotADirectoryError:
        print(f"{path} is not a folder.")
        exit(1)

    Handler = CustomHTTPRequestHandler

    with socketserver.TCPServer(("", port), Handler, bind_and_activate=False) as httpd:
        # we need to set this before binding and activating or you have to wait
        # a minute before it will let you restart the server on the same port
        # after killing it
        httpd.allow_reuse_address = True

        httpd.server_bind()
        httpd.server_activate()

        print(f"serving {path} at port {port}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            httpd.shutdown()
