#!/usr/bin/env python3
import http.server
import socketserver
import os
import json
import urllib.parse

PORT = 8000
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
UPLOAD_DIR = os.path.join(SCRIPT_DIR, "uploads")

os.makedirs(UPLOAD_DIR, exist_ok=True)

class FileUploadHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path.startswith('/api/files'):
            self.send_file_list()
            return
        elif self.path.startswith('/api/download/'):
            filename = urllib.parse.unquote(self.path.split('/api/download/')[-1])
            self.send_file(filename)
            return
        else:
            self.send_error(404, "Not Found")
    
    def do_POST(self):
        if self.path == '/api/upload':
            self.handle_upload()
        else:
            self.send_error(404, "Not Found")
    
    def handle_upload(self):
        try:
            content_length = int(self.headers.get('Content-Length', 0))
            if content_length == 0:
                self.send_response(400)
                self.send_header('Content-Type', 'application/json')
                self.send_header('Access-Control-Allow-Origin', '*')
                self.end_headers()
                self.wfile.write(json.dumps({'success': False, 'error': 'No content'}).encode())
                return
                
            content_type = self.headers.get('Content-Type', '')
            
            if 'multipart/form-data' in content_type:
                if 'boundary=' not in content_type:
                    self.send_response(400)
                    self.send_header('Content-Type', 'application/json')
                    self.send_header('Access-Control-Allow-Origin', '*')
                    self.end_headers()
                    self.wfile.write(json.dumps({'success': False, 'error': 'Invalid boundary'}).encode())
                    return
                    
                boundary = content_type.split('boundary=')[1].encode()
                data = self.rfile.read(content_length)
                
                parts = data.split(b'--' + boundary)
                for part in parts:
                    if b'Content-Disposition: form-data' in part:
                        header_end = part.find(b'\r\n\r\n')
                        if header_end == -1:
                            continue
                        header = part[:header_end].decode('utf-8', errors='ignore')
                        file_data = part[header_end+4:]
                        
                        if 'filename=' in header:
                            filename_start = header.find('filename="') + 10
                            if filename_start < 10:
                                continue
                            filename_end = header.find('"', filename_start)
                            if filename_end == -1:
                                continue
                            filename = header[filename_start:filename_end]
                            
                            file_data = file_data.rstrip(b'\r\n--')
                            
                            filepath = os.path.join(UPLOAD_DIR, filename)
                            with open(filepath, 'wb') as f:
                                f.write(file_data)
                            
                            self.send_response(200)
                            self.send_header('Content-Type', 'application/json')
                            self.send_header('Access-Control-Allow-Origin', '*')
                            self.end_headers()
                            self.wfile.write(json.dumps({
                                'success': True,
                                'filename': filename,
                                'size': len(file_data)
                            }).encode())
                            return
            
            self.send_response(400)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(json.dumps({'success': False, 'error': 'Invalid request'}).encode())
        except Exception as e:
            self.send_response(500)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(json.dumps({'success': False, 'error': str(e)}).encode())
    
    def send_file_list(self):
        files = []
        try:
            if os.path.exists(UPLOAD_DIR):
                for f in os.listdir(UPLOAD_DIR):
                    filepath = os.path.join(UPLOAD_DIR, f)
                    if os.path.isfile(filepath):
                        files.append({
                            'name': f,
                            'size': os.path.getsize(filepath)
                        })
        except Exception as e:
            print(f"Error listing files: {e}")
        
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(json.dumps(files).encode())
    
    def send_file(self, filename):
        if not filename or '..' in filename or '/' in filename or '\\' in filename:
            self.send_error(400, "Invalid filename")
            return
            
        filepath = os.path.join(UPLOAD_DIR, filename)
        if not os.path.exists(filepath) or not os.path.isfile(filepath):
            self.send_error(404, "File not found")
            return
        
        try:
            file_size = os.path.getsize(filepath)
            self.send_response(200)
            self.send_header('Content-Type', 'application/octet-stream')
            self.send_header('Content-Disposition', f'attachment; filename="{filename}"')
            self.send_header('Content-Length', str(file_size))
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            
            with open(filepath, 'rb') as f:
                while True:
                    chunk = f.read(8192)
                    if not chunk:
                        break
                    self.wfile.write(chunk)
        except Exception as e:
            print(f"Error sending file: {e}")
            try:
                self.send_error(500, "Error reading file")
            except:
                pass
    
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        super().end_headers()
    
    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()
    
    def log_message(self, format, *args):
        pass

def main():
    print(f"Server starting on http://0.0.0.0:{PORT}")
    print(f"Upload directory: {os.path.abspath(UPLOAD_DIR)}")
    print(f"Access from other PCs: http://<SERVER_IP>:{PORT}")
    
    Handler = FileUploadHandler
    with socketserver.TCPServer(("0.0.0.0", PORT), Handler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped")

if __name__ == "__main__":
    main()

