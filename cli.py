#!/usr/bin/env python3
"""CLI tool for DRS-SYNC"""
import argparse
import asyncio
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from backend.chunk_manager import ChunkManager
from backend.interface_scanner import InterfaceScanner
from backend.config import UPLOAD_DIR


async def cmd_scan():
    """Scan network interfaces"""
    scanner = InterfaceScanner()
    links = await scanner.scan_all_interfaces()
    
    print("\nNetwork Interfaces:")
    print("=" * 80)
    print(f"{'Interface':<15} {'IP Address':<18} {'Score':<8} {'Throughput':<12} {'RTT':<8} {'Loss':<8}")
    print("-" * 80)
    
    for link in links:
        print(f"{link.interface:<15} {link.ip_address:<18} {link.link_score:>6.1%} "
              f"{link.throughput_mbps:>10.2f} Mbps {link.rtt_ms:>6.1f}ms "
              f"{link.packet_loss:>6.1%}")
    
    best = scanner.get_best_link()
    if best:
        print(f"\nBest Link: {best.interface} (Score: {best.link_score:.1%})")


async def cmd_upload(file_path: str, priority: str = "standard"):
    """Upload a file"""
    file_path = Path(file_path)
    if not file_path.exists():
        print(f"Error: File not found: {file_path}")
        return
    
    import requests
    
    url = "http://localhost:8080/api/files/upload"
    with open(file_path, 'rb') as f:
        files = {'file': (file_path.name, f)}
        data = {'priority': priority}
        response = requests.post(url, files=files, data=data)
    
    if response.status_code == 200:
        result = response.json()
        print(f"\nFile uploaded successfully!")
        print(f"  File ID: {result['file_id']}")
        print(f"  Size: {result['size'] / 1024 / 1024:.2f} MB")
        print(f"  Chunks: {result['total_chunks']}")
        print(f"  Priority: {result['priority']}")
    else:
        print(f"Error: {response.status_code} - {response.text}")


async def cmd_status(file_id: str = None):
    """Get transfer status"""
    import requests
    
    if file_id:
        url = f"http://localhost:8080/api/files/{file_id}"
    else:
        url = "http://localhost:8080/api/files"
    
    response = requests.get(url)
    if response.status_code == 200:
        data = response.json()
        if file_id:
            # Single file
            print(f"\nFile: {data.get('filename', 'N/A')}")
            print(f"Size: {data.get('size', 0) / 1024 / 1024:.2f} MB")
            print(f"Priority: {data.get('priority', 'N/A')}")
            if 'progress' in data:
                p = data['progress']
                print(f"Progress: {p.get('progress', 0):.1%}")
                print(f"Transferred: {p.get('bytes_transferred', 0) / 1024 / 1024:.2f} MB")
        else:
            # All files
            files = data.get('files', [])
            print(f"\nFiles in queue: {len(files)}")
            for f in files:
                print(f"  {f['file_id']}: {f['filename']} - {f['status']}")
    else:
        print(f"Error: {response.status_code}")


async def cmd_transfer(file_id: str, host: str = "localhost", port: int = 9000):
    """Start a transfer"""
    import requests
    
    url = f"http://localhost:8080/api/files/{file_id}/transfer"
    data = {
        "file_id": file_id,
        "destination_host": host,
        "destination_port": port
    }
    
    response = requests.post(url, json=data)
    if response.status_code == 200:
        print(f"Transfer started: {file_id} -> {host}:{port}")
    else:
        print(f"Error: {response.status_code} - {response.text}")


def main():
    parser = argparse.ArgumentParser(description="DRS-SYNC CLI")
    subparsers = parser.add_subparsers(dest='command', help='Command')
    
    # Scan command
    subparsers.add_parser('scan', help='Scan network interfaces')
    
    # Upload command
    upload_parser = subparsers.add_parser('upload', help='Upload a file')
    upload_parser.add_argument('file', help='File path')
    upload_parser.add_argument('--priority', choices=['high', 'standard', 'background'],
                              default='standard', help='Priority level')
    
    # Status command
    status_parser = subparsers.add_parser('status', help='Get transfer status')
    status_parser.add_argument('file_id', nargs='?', help='File ID (optional)')
    
    # Transfer command
    transfer_parser = subparsers.add_parser('transfer', help='Start a transfer')
    transfer_parser.add_argument('file_id', help='File ID')
    transfer_parser.add_argument('--host', default='localhost', help='Destination host')
    transfer_parser.add_argument('--port', type=int, default=9000, help='Destination port')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return
    
    if args.command == 'scan':
        asyncio.run(cmd_scan())
    elif args.command == 'upload':
        asyncio.run(cmd_upload(args.file, args.priority))
    elif args.command == 'status':
        asyncio.run(cmd_status(args.file_id))
    elif args.command == 'transfer':
        asyncio.run(cmd_transfer(args.file_id, args.host, args.port))


if __name__ == "__main__":
    main()

