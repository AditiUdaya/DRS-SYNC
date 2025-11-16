#!/usr/bin/env python3
import urllib.request
import sys
import os

def download_file(url, output_file):
    try:
        print(f"Downloading from {url}...")
        urllib.request.urlretrieve(url, output_file)
        print(f"Download complete: {output_file}")
        file_size = os.path.getsize(output_file)
        print(f"File size: {file_size} bytes")
        return True
    except Exception as e:
        print(f"Error: {e}")
        return False

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 client_http.py <server_url> [output_file]")
        print("Example: python3 client_http.py http://192.168.1.100:8000/myfile.zip")
        sys.exit(1)
    
    url = sys.argv[1]
    if len(sys.argv) > 2:
        output_file = sys.argv[2]
    else:
        output_file = os.path.basename(url.split('/')[-1])
        if not output_file:
            output_file = "downloaded.zip"
    
    if download_file(url, output_file):
        print("Transfer successful!")
    else:
        print("Transfer failed!")
        sys.exit(1)

if __name__ == "__main__":
    main()

