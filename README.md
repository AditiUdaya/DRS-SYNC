# DRS-SYNC

High-performance file transfer system with congestion control, integrity checking, and checkpoint/resume capabilities.

## Project Structure

```
DRS-SYNC/
├── include/drs_sync/          # Core library headers
│   ├── packet.hpp             # UDP packet structure
│   ├── network_interface.hpp   # UDP network layer
│   ├── transfer_engine.hpp     # Main transfer orchestration
│   ├── congestion_control.hpp  # Congestion control algorithms
│   ├── simple_checkpoint.hpp  # Checkpoint/resume functionality
│   └── integrity.hpp           # File integrity verification
├── src/                        # Core library implementation
│   ├── network_interface.cpp
│   ├── transfer_engine.cpp
│   ├── congestion_control.cpp
│   ├── simple_checkpoint.cpp
│   ├── integrity.cpp
│   ├── api_server_light.cpp    # Lightweight HTTP API (cpp-httplib)
│   └── api_server_mongoose.cpp  # Alternative API server (mongoose)
├── external/                   # Header-only dependencies
│   ├── httplib.h              # cpp-httplib (HTTP server)
│   ├── json.hpp               # nlohmann/json (JSON parsing)
│   ├── mongoose.h             # Mongoose web server
│   └── mongoose.c
├── frontend/                   # React frontend application
│   ├── package.json
│   ├── vite.config.js
│   ├── index.html
│   └── src/
│       ├── main.jsx
│       ├── App.jsx
│       ├── App.css
│       └── components/
│           ├── Dashboard.jsx
│           ├── Speedometer.jsx
│           ├── TelemetryPanel.jsx
│           └── FileUploader.jsx
├── build/                      # Build artifacts (generated)
├── CMakeLists.txt             # CMake build configuration
└── README.md
```

## Features

- **High-Performance UDP Transfer**: Optimized for large file transfers
- **Congestion Control**: Adaptive rate limiting based on network conditions
- **Integrity Checking**: SHA-256 verification of transferred data
- **Checkpoint/Resume**: Resume interrupted transfers from last checkpoint
- **Priority Levels**: NORMAL, HIGH, and CRITICAL transfer priorities
- **RESTful API**: HTTP API for managing transfers
- **Web Dashboard**: React-based frontend for monitoring and control

## Dependencies

### System Requirements
- C++17 compatible compiler
- CMake 3.15+
- OpenSSL (for HTTPS support)
- Asio library (header-only, via Homebrew on macOS)
- SQLite3 (for checkpoint storage)

### macOS Installation
```bash
brew install cmake openssl asio sqlite3
```

## Building

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# The executable will be at: build/drs_sync_api
```

## Running

### Start the API Server
```bash
cd build
./drs_sync_api
```

The server will start on `http://localhost:8080` by default.

## API Endpoints

### Root
- **GET /** - API information and available endpoints

### Health Check
- **GET /ping** - Health check endpoint

### Transfer Management
- **POST /manifest/create** - Create a new file transfer
  ```json
  {
    "filepath": "/path/to/file",
    "destination": "host:port",
    "priority": "NORMAL"  // or "HIGH", "CRITICAL"
  }
  ```

- **GET /transfer/:id/status** - Get transfer status and statistics
- **POST /transfer/:id/pause** - Pause a running transfer
- **POST /transfer/:id/resume** - Resume a paused transfer

### Example: Create a Transfer
```bash
curl -X POST http://localhost:8080/manifest/create \
  -H "Content-Type: application/json" \
  -d '{
    "filepath": "/path/to/file.txt",
    "destination": "192.168.1.100:9091",
    "priority": "NORMAL"
  }'
```

### Example: Check Transfer Status
```bash
curl http://localhost:8080/transfer/1234567890/status
```

## Frontend

The React frontend provides a web-based dashboard for monitoring transfers.

```bash
cd frontend
npm install
npm run dev
```

The frontend will be available at `http://localhost:5173` (Vite default port).

## Architecture

- **Transfer Engine**: Orchestrates file transfers with multiple worker threads
- **Network Interface**: Handles UDP packet sending/receiving
- **Congestion Control**: Implements adaptive window sizing and rate limiting
- **Checkpoint System**: Saves transfer state to SQLite for resume capability
- **Integrity Module**: Verifies file integrity using SHA-256 hashing

## License

[Add your license here]
