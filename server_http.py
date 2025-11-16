#!/usr/bin/env python3
import http.server
import socketserver
import os
import sys

PORT = 8000

class ZipFileHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        super().end_headers()

def main():
    if len(sys.argv) > 1:
        zip_file = sys.argv[1]
        if not os.path.exists(zip_file):
            print(f"Error: File '{zip_file}' not found")
            sys.exit(1)
        os.chdir(os.path.dirname(os.path.abspath(zip_file)))
        print(f"Serving directory: {os.getcwd()}")
    else:
        print("Usage: python3 server_http.py <zip_file>")
        print("Example: python3 server_http.py myfile.zip")
        sys.exit(1)
    
    Handler = ZipFileHandler
    with socketserver.TCPServer(("0.0.0.0", PORT), Handler) as httpd:
        print(f"HTTP Server running on http://0.0.0.0:{PORT}")
        print(f"Access from other PCs: http://<SERVER_IP>:{PORT}/{os.path.basename(zip_file)}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped")

if __name__ == "__main__":
    main()

