#!/usr/bin/env python3
import socket
import sys
import os

PORT = 9999

def receive_file(conn, output_file):
    # Receive header (filename|size\n)
    header = b""
    while b"\n" not in header:
        chunk = conn.recv(1)
        if not chunk:
            return False
        header += chunk
    
    header_str = header.decode().strip()
    parts = header_str.split("|")
    if len(parts) != 2:
        return False
    
    filename, file_size_str = parts
    file_size = int(file_size_str)
    
    if not output_file:
        output_file = filename
    
    print(f"Receiving: {filename} ({file_size} bytes)")
    
    # Receive file data
    received = 0
    with open(output_file, 'wb') as f:
        while received < file_size:
            chunk = conn.recv(min(8192, file_size - received))
            if not chunk:
                break
            f.write(chunk)
            received += len(chunk)
    
    return received == file_size

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 client_tcp.py <server_ip> [output_file]")
        print("Example: python3 client_tcp.py 192.168.1.100")
        sys.exit(1)
    
    server_ip = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        print(f"Connecting to {server_ip}:{PORT}...")
        sock.connect((server_ip, PORT))
        print("Connected to server")
        
        if receive_file(sock, output_file):
            if output_file:
                file_size = os.path.getsize(output_file)
                print(f"File saved: {output_file} ({file_size} bytes)")
            print("Transfer successful!")
        else:
            print("Transfer failed!")
            sys.exit(1)
            
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
    finally:
        sock.close()

if __name__ == "__main__":
    main()

