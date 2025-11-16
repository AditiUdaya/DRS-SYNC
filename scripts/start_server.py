#!/usr/bin/env python3
"""Start the DRS-SYNC server"""
import sys
import os
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

import uvicorn
from backend.api import app
from backend.config import API_HOST, API_PORT

if __name__ == "__main__":
    # Allow port to be specified as command line argument
    port = int(sys.argv[1]) if len(sys.argv) > 1 else API_PORT
    
    print("🚦 Starting DRS-SYNC Server...")
    print(f"API: http://{API_HOST}:{port}")
    print(f"WebSocket: ws://{API_HOST}:{port}/ws")
    print("\nPress Ctrl+C to stop\n")
    
    try:
        uvicorn.run(
            app,
            host=API_HOST,
            port=port,
            log_level="info"
        )
    except OSError as e:
        if e.errno == 48:  # Address already in use
            print(f"\n❌ Error: Port {port} is already in use.")
            print(f"   Solutions:")
            print(f"   1. Use a different port: python3 scripts/start_server.py 8081")
            print(f"   2. Kill the process: lsof -ti:{port} | xargs kill")
            print(f"   3. Check what's using it: lsof -i:{port}")
            sys.exit(1)
        else:
            raise