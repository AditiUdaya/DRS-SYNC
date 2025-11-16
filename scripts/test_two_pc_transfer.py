#!/usr/bin/env python3
"""Test script for two-PC DRS-SYNC transfer"""
import sys
import time
import requests
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

API_BASE = "http://localhost:8080/api"

def create_test_file(filename: str, size_mb: float = 1.0) -> Path:
    """Create a test file with some compressible data"""
    test_dir = Path(__file__).parent.parent / "test_files"
    test_dir.mkdir(exist_ok=True)
    
    file_path = test_dir / filename
    size_bytes = int(size_mb * 1024 * 1024)
    
    # Create file with compressible data (repeating patterns)
    with open(file_path, 'wb') as f:
        pattern = b"DRS-SYNC-TEST-PATTERN-" * 100
        while f.tell() < size_bytes:
            remaining = size_bytes - f.tell()
            f.write(pattern[:remaining])
    
    print(f"✅ Created test file: {file_path} ({size_mb}MB)")
    return file_path

def upload_file(file_path: Path, priority: str = "high") -> str:
    """Upload a file to the server"""
    print(f"\n📤 Uploading {file_path.name}...")
    
    try:
        with open(file_path, 'rb') as f:
            files = {'file': (file_path.name, f, 'application/octet-stream')}
            data = {'priority': priority}
            
            response = requests.post(f"{API_BASE}/files/upload", files=files, data=data, timeout=30)
            response.raise_for_status()
            
            result = response.json()
            file_id = result['file_id']
            print(f"✅ Uploaded! File ID: {file_id}")
            return file_id
    except requests.exceptions.ConnectionError:
        print(f"❌ Error: Could not connect to backend server at {API_BASE}")
        print(f"   Make sure the backend server is running:")
        print(f"   python3 scripts/start_server.py")
        sys.exit(1)
    except Exception as e:
        print(f"❌ Upload failed: {e}")
        sys.exit(1)

def start_transfer(file_id: str, host: str, port: int):
    """Start a transfer"""
    print(f"\n🚀 Starting transfer for {file_id} to {host}:{port}...")
    
    try:
        response = requests.post(
            f"{API_BASE}/files/{file_id}/transfer",
            json={
                "file_id": file_id,
                "destination_host": host,
                "destination_port": port
            },
            timeout=10
        )
        response.raise_for_status()
        print(f"✅ Transfer started!")
    except Exception as e:
        print(f"❌ Failed to start transfer: {e}")
        sys.exit(1)

def get_status(file_id: str):
    """Get transfer status"""
    try:
        response = requests.get(f"{API_BASE}/files/{file_id}", timeout=5)
        response.raise_for_status()
        return response.json()
    except Exception as e:
        print(f"⚠️  Error getting status: {e}")
        return None

def wait_for_completion(file_id: str, timeout: int = 300):
    """Wait for transfer to complete"""
    print(f"\n⏳ Waiting for transfer to complete...")
    print(f"   (Timeout: {timeout}s)")
    print()
    start_time = time.time()
    last_progress = -1
    
    while time.time() - start_time < timeout:
        status = get_status(file_id)
        if not status:
            time.sleep(2)
            continue
        
        progress = status.get('progress', {})
        percent = progress.get('progress', 0) * 100
        bytes_transferred = progress.get('bytes_transferred', 0)
        bytes_total = progress.get('bytes_total', 0)
        
        # Only print when progress changes
        if int(percent) != last_progress:
            mb_transferred = bytes_transferred / (1024 * 1024)
            mb_total = bytes_total / (1024 * 1024)
            print(f"  Progress: {percent:5.1f}% ({mb_transferred:.2f}/{mb_total:.2f} MB)", end='\r')
            last_progress = int(percent)
        
        # Check if complete
        transfer_status = status.get('status', {})
        if not transfer_status.get('is_active', True) and percent >= 100:
            print(f"\n✅ Transfer completed!")
            return True
        
        time.sleep(1)
    
    print(f"\n❌ Transfer timed out after {timeout}s!")
    return False

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 scripts/test_two_pc_transfer.py <RECEIVER_IP> <PORT> [FILE_SIZE_MB]")
        print()
        print("Example:")
        print("  python3 scripts/test_two_pc_transfer.py 192.168.1.100 9000")
        print("  python3 scripts/test_two_pc_transfer.py 192.168.1.100 9000 5.0")
        print()
        print("Make sure:")
        print("  1. Receiver is running on the other PC:")
        print("     python3 -m backend.receiver 9000")
        print("  2. Backend server is running on this PC:")
        print("     python3 scripts/start_server.py")
        sys.exit(1)
    
    receiver_ip = sys.argv[1]
    receiver_port = int(sys.argv[2])
    file_size = float(sys.argv[3]) if len(sys.argv) > 3 else 2.0
    
    print("=" * 70)
    print("DRS-SYNC Two-PC Transfer Test")
    print("=" * 70)
    print(f"Receiver: {receiver_ip}:{receiver_port}")
    print(f"File size: {file_size}MB")
    print("=" * 70)
    
    # Verify backend is running
    try:
        response = requests.get(f"{API_BASE}/", timeout=2)
        if response.status_code != 200:
            print(f"❌ Backend server is not responding correctly")
            sys.exit(1)
    except requests.exceptions.ConnectionError:
        print(f"❌ Cannot connect to backend server at {API_BASE}")
        print(f"   Please start the backend server first:")
        print(f"   python3 scripts/start_server.py")
        sys.exit(1)
    
    # Create test file
    print("\n📁 Creating test file...")
    test_file = create_test_file("two_pc_test.bin", size_mb=file_size)
    
    # Upload file
    file_id = upload_file(test_file, priority="high")
    
    # Start transfer
    start_transfer(file_id, receiver_ip, receiver_port)
    
    # Wait for completion
    success = wait_for_completion(file_id, timeout=600)
    
    # Final status
    if success:
        status = get_status(file_id)
        if status:
            progress = status.get('progress', {})
            stats = status.get('status', {})
            print()
            print("=" * 70)
            print("Transfer Summary")
            print("=" * 70)
            print(f"File ID: {file_id}")
            print(f"Progress: {progress.get('progress', 0) * 100:.1f}%")
            print(f"Bytes transferred: {progress.get('bytes_transferred', 0):,}")
            print(f"Chunks sent: {stats.get('chunks_sent', 0)}")
            print(f"Chunks ACKed: {stats.get('chunks_acked', 0)}")
            if stats.get('compression_ratio'):
                print(f"Compression ratio: {stats.get('compression_ratio', 0):.2%}")
            print("=" * 70)
            print()
            print("✅ Test completed successfully!")
            print(f"   Check the receiver PC for the received file in the 'received/' directory")
        else:
            print("✅ Transfer completed (could not get final status)")
    else:
        print()
        print("❌ Transfer did not complete within timeout")
        print("   Check:")
        print("   1. Receiver is still running on the other PC")
        print("   2. Network connectivity between PCs")
        print("   3. Firewall settings")
        print("   4. Backend server logs for errors")
        sys.exit(1)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n❌ Test interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

