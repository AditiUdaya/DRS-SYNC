#!/usr/bin/env python3
"""Test script for 2-client DRS-SYNC transfer"""
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
    
    with open(file_path, 'rb') as f:
        files = {'file': (file_path.name, f, 'application/octet-stream')}
        data = {'priority': priority}
        
        response = requests.post(f"{API_BASE}/files/upload", files=files, data=data)
        response.raise_for_status()
        
        result = response.json()
        file_id = result['file_id']
        print(f"✅ Uploaded! File ID: {file_id}")
        return file_id

def start_transfer(file_id: str, host: str = "localhost", port: int = 9000):
    """Start a transfer"""
    print(f"\n🚀 Starting transfer for {file_id} to {host}:{port}...")
    
    response = requests.post(
        f"{API_BASE}/files/{file_id}/transfer",
        json={
            "file_id": file_id,
            "destination_host": host,
            "destination_port": port
        }
    )
    response.raise_for_status()
    print(f"✅ Transfer started!")

def get_status(file_id: str):
    """Get transfer status"""
    response = requests.get(f"{API_BASE}/files/{file_id}")
    response.raise_for_status()
    return response.json()

def wait_for_completion(file_id: str, timeout: int = 300):
    """Wait for transfer to complete"""
    print(f"\n⏳ Waiting for transfer to complete...")
    start_time = time.time()
    
    while time.time() - start_time < timeout:
        status = get_status(file_id)
        progress = status.get('progress', {})
        percent = progress.get('progress', 0) * 100
        bytes_transferred = progress.get('bytes_transferred', 0)
        bytes_total = progress.get('bytes_total', 0)
        
        print(f"  Progress: {percent:.1f}% ({bytes_transferred}/{bytes_total} bytes)", end='\r')
        
        if status.get('status', {}).get('is_active') == False and percent >= 100:
            print(f"\n✅ Transfer completed!")
            return True
        
        time.sleep(1)
    
    print(f"\n❌ Transfer timed out!")
    return False

def main():
    print("=" * 60)
    print("DRS-SYNC Two-Client Test")
    print("=" * 60)
    
    # Create test files
    print("\n📁 Creating test files...")
    file1 = create_test_file("client1_test.bin", size_mb=2.0)
    file2 = create_test_file("client2_test.bin", size_mb=1.5)
    
    # Client 1: Upload and transfer file 1
    print("\n" + "=" * 60)
    print("CLIENT 1: Uploading and transferring file 1")
    print("=" * 60)
    file_id_1 = upload_file(file1, priority="high")
    start_transfer(file_id_1, host="localhost", port=9000)
    
    # Client 2: Upload and transfer file 2 (to different port)
    print("\n" + "=" * 60)
    print("CLIENT 2: Uploading and transferring file 2")
    print("=" * 60)
    file_id_2 = upload_file(file2, priority="standard")
    start_transfer(file_id_2, host="localhost", port=9001)
    
    # Wait for both transfers
    print("\n" + "=" * 60)
    print("Waiting for both transfers to complete...")
    print("=" * 60)
    
    import threading
    
    def wait_for_file1():
        wait_for_completion(file_id_1)
    
    def wait_for_file2():
        wait_for_completion(file_id_2)
    
    thread1 = threading.Thread(target=wait_for_file1)
    thread2 = threading.Thread(target=wait_for_file2)
    
    thread1.start()
    thread2.start()
    
    thread1.join()
    thread2.join()
    
    print("\n" + "=" * 60)
    print("✅ Test completed!")
    print("=" * 60)
    print("\nTo run this test:")
    print("1. Start receiver 1: python3 -m backend.receiver 9000")
    print("2. Start receiver 2: python3 -m backend.receiver 9001")
    print("3. Start backend: python3 scripts/start_server.py")
    print("4. Run this script: python3 scripts/test_two_clients.py")

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

