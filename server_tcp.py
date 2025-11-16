#!/usr/bin/env python3
import socket
import os
import sys

PORT = 9999

def send_file(conn, filepath):
    file_size = os.path.getsize(filepath)
    filename = os.path.basename(filepath)
    
    # Send filename and size
    header = f"{filename}|{file_size}\n".encode()
    conn.sendall(header)
    
    # Send file data
    with open(filepath, 'rb') as f:
        sent = 0
        while sent < file_size:
            chunk = f.read(8192)
            if not chunk:
                break
            conn.sendall(chunk)
            sent += len(chunk)
    
    return sent

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 server_tcp.py <zip_file>")
        print("Example: python3 server_tcp.py myfile.zip")
        sys.exit(1)
    
    zip_file = sys.argv[1]
    if not os.path.exists(zip_file):
        print(f"Error: File '{zip_file}' not found")
        sys.exit(1)
    
    zip_file = os.path.abspath(zip_file)
    file_size = os.path.getsize(zip_file)
    print(f"Server ready. File: {os.path.basename(zip_file)} ({file_size} bytes)")
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        sock.bind(("0.0.0.0", PORT))
        sock.listen(1)
        print(f"TCP Server listening on 0.0.0.0:{PORT}")
        print("Waiting for client connection...")
        
        conn, addr = sock.accept()
        print(f"Client connected from {addr[0]}:{addr[1]}")
        
        bytes_sent = send_file(conn, zip_file)
        print(f"File sent: {bytes_sent} bytes")
        
        conn.close()
        print("Transfer complete. Server closing.")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    main()

