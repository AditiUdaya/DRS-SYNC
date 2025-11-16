# 🚦 DRS-SYNC: Intelligent Multi-Uplink File Transfer Engine

**TrackShift 2025** - A real-time, intelligent, multi-uplink data relay system designed for motorsport telemetry.

## Overview

DRS-SYNC reliably transfers large data files/streams (car telemetry, video, LiDAR, race strategy packets) from trackside systems to the cloud or pit computers even under unstable or changing network conditions.

### Key Features

- **Multi-Uplink Performance Scanning** - Automatically scans all network interfaces and measures throughput, RTT, packet loss, jitter, and stability
- **Adaptive Uplink Selection** - Automatically selects the best performing link based on real-time metrics
- **Reliable Chunked Transfer** - Lossless, resumable transfers with sliding window and adaptive send rate
- **Fault Tolerance** - Automatic resume from checkpoints after crashes or network disconnections
- **Network Simulation** - Built-in simulator for testing flaky network conditions
- **Real-Time Dashboard** - Motorsport-themed React dashboard with live metrics and transfer visualization

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    React Dashboard (Frontend)                │
│  ┌──────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │File Queue│  │Transfer Canvas│  │   Metrics Panel      │  │
│  └──────────┘  └──────────────┘  └──────────────────────┘  │
└───────────────────────┬─────────────────────────────────────┘
                        │ WebSocket + REST API
┌───────────────────────┴─────────────────────────────────────┐
│              FastAPI Backend (Python)                        │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Transfer Engine (Sliding Window + Retransmission)   │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Interface Scanner (Multi-Uplink Performance Test)   │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Chunk Manager (Manifest + Checkpointing)            │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Network Simulator (Packet Loss, Latency, Jitter)    │   │
│  └──────────────────────────────────────────────────────┘   │
└───────────────────────┬─────────────────────────────────────┘
                        │ UDP (Interface-Bound Sockets)
        ┌───────────────┴───────────────┐
        │                               │
   ┌────▼────┐  ┌──────┐  ┌──────────┐ │
   │ Ethernet│  │ WiFi │  │ 5G/4G   │ │
   └─────────┘  └──────┘  └──────────┘ │
```

## Quick Start

### Prerequisites

- Python 3.8+
- Node.js 16+ (for frontend)
- Network interfaces (WiFi, Ethernet, etc.)

### Installation

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd DRS-SYNC
   ```

2. **Set up Python backend**
   ```bash
   python3 -m venv venv
   source venv/bin/activate  # On Windows: venv\Scripts\activate
   pip install -r requirements.txt
   ```

3. **Set up React frontend**
   ```bash
   cd frontend
   npm install
   ```

### Testing Options

- **Single PC Testing**: See [QUICKSTART.md](QUICKSTART.md) for running everything on one machine
- **Two-PC Testing**: See [TWO_PC_SETUP.md](TWO_PC_SETUP.md) for testing transfers between two separate computers

### Running the System

1. **Start the receiver** (in one terminal)
   ```bash
   python3 -m backend.receiver
   ```

2. **Start the backend server** (in another terminal)
   ```bash
   python3 scripts/start_server.py
   ```

3. **Start the frontend** (in another terminal)
   ```bash
   cd frontend
   npm start
   ```

4. **Access the dashboard**
   - Open http://localhost:3000 in your browser

### Using the Demo Script

Run the complete demo suite:
```bash
chmod +x scripts/start_demo.sh
./scripts/start_demo.sh
```

Or run individual demos:
```bash
python3 scripts/demo.py
```

## CLI Usage

The DRS-SYNC CLI provides command-line access to all features:

```bash
# Scan network interfaces
python3 cli.py scan

# Upload a file
python3 cli.py upload /path/to/file.bin --priority high

# Check transfer status
python3 cli.py status
python3 cli.py status <file_id>

# Start a transfer
python3 cli.py transfer <file_id> --host localhost --port 9000
```

## API Documentation

### REST Endpoints

- `GET /api/links` - Get all network links and metrics
- `GET /api/links/best` - Get the best performing link
- `POST /api/files/upload` - Upload a file to the queue
- `GET /api/files` - List all files
- `GET /api/files/{file_id}` - Get file status
- `POST /api/files/{file_id}/transfer` - Start a transfer
- `POST /api/files/{file_id}/pause` - Pause a transfer
- `POST /api/files/{file_id}/resume` - Resume a transfer
- `POST /api/files/{file_id}/cancel` - Cancel a transfer
- `PUT /api/files/{file_id}/priority` - Update file priority
- `POST /api/simulator/config` - Configure network simulator
- `POST /api/simulator/kill-link` - Kill a link (simulate failure)
- `POST /api/simulator/restore-link` - Restore a killed link

### WebSocket

Connect to `ws://localhost:8080/ws` to receive real-time telemetry:
- `link_metrics` - Periodic link performance updates
- `transfer_progress` - Real-time transfer progress updates

## Demo Scenarios

### Demo 1: Baseline Burst
Upload a 500MB telemetry file and watch it transfer at full speed.

### Demo 2: Kill the Uplink
Simulate 70% packet loss and watch the system automatically adapt and slow down gracefully.

### Demo 3: Disconnect the Sender
Kill the transfer process, restart it, and watch it resume from the last checkpoint.

### Demo 4: Multi-Uplink Switching
Turn off WiFi and watch the system automatically switch to the 4G hotspot.

### Demo 5: Multipath Boost (Future)
Send chunks over WiFi + Ethernet simultaneously for increased throughput.

## Configuration

Edit `backend/config.py` to customize:

- `CHUNK_SIZE` - Size of each chunk (default: 64KB)
- `SLIDING_WINDOW_SIZE` - Number of chunks in flight (default: 10)
- `MAX_RETRIES` - Maximum retry attempts per chunk (default: 3)
- `SCORE_WEIGHTS` - Weights for link scoring algorithm
- `SCAN_INTERVAL` - How often to rescan interfaces (default: 5s)

## Project Structure

```
DRS-SYNC/
├── backend/                 # Python backend
│   ├── __init__.py
│   ├── api.py              # FastAPI REST + WebSocket server
│   ├── config.py           # Configuration
│   ├── interface_scanner.py # Multi-uplink scanner
│   ├── chunk_manager.py    # Chunk & manifest management
│   ├── transfer_engine.py  # Core transfer engine
│   ├── network_simulator.py # Network condition simulator
│   └── receiver.py         # UDP receiver for testing
├── frontend/               # React dashboard
│   ├── src/
│   │   ├── App.js
│   │   ├── components/     # React components
│   │   └── services/       # API & WebSocket clients
│   └── package.json
├── scripts/
│   ├── start_server.py     # Server startup script
│   ├── demo.py             # Demo scenarios
│   └── start_demo.sh       # Complete demo launcher
├── data/                   # Runtime data
│   ├── manifests/          # Transfer manifests
│   └── cache/              # Cached chunks
├── uploads/                # Uploaded files
├── received/               # Received files
├── cli.py                  # Command-line interface
├── requirements.txt        # Python dependencies
└── README.md
```

## Link Scoring Algorithm

The system uses a weighted scoring algorithm to rank links:

```
score = (throughput_weight × throughput_norm) +
        (rtt_weight × rtt_norm) +
        (loss_weight × loss_norm) +
        (stability_weight × stability_score)
```

Where:
- `throughput_norm`: Normalized throughput (0-100 Mbps → 0-1)
- `rtt_norm`: Normalized RTT (0-200ms → 1-0, lower is better)
- `loss_norm`: 1 - packet_loss (0-1)
- `stability_score`: Based on jitter and loss (0-1)

Default weights:
- Throughput: 40%
- RTT: 30%
- Loss: 20%
- Stability: 10%

## Transfer Protocol

### Chunk Packet Format

```
[file_id_len (1 byte)]
[file_id (variable)]
[chunk_id (4 bytes, big-endian)]
[offset (8 bytes, big-endian)]
[size (4 bytes, big-endian)]
[hash (8 bytes, xxHash64)]
[data (variable)]
```

### Manifest Format

Manifests are stored as JSON files in `data/manifests/`:

```json
{
  "file_id": "uuid",
  "file_path": "/path/to/file",
  "file_size": 10485760,
  "file_hash": "sha256...",
  "total_chunks": 160,
  "chunks": {
    "0": {
      "chunk_id": 0,
      "offset": 0,
      "size": 65536,
      "hash": "xxhash...",
      "status": "acked",
      "retry_count": 0
    }
  },
  "priority": "high",
  "created_at": "2025-01-01T12:00:00",
  "updated_at": "2025-01-01T12:05:00"
}
```

## Troubleshooting

### No network interfaces found
- Ensure you have active network interfaces (WiFi, Ethernet)
- On Linux, you may need to run with `sudo` for interface binding
- Check that interfaces are up: `ip link show` or `ifconfig`

### Transfer not starting
- Check that the receiver is running on the destination port
- Verify network connectivity to destination
- Check firewall settings

### WebSocket connection failed
- Ensure the backend server is running on port 8080
- Check CORS settings if accessing from a different origin
- Verify WebSocket support in your browser

## Development

### Running Tests

```bash
# Backend tests (when implemented)
pytest backend/tests/

# Frontend tests
cd frontend
npm test
```

### Code Style

- Python: Follow PEP 8
- JavaScript: ESLint with React rules

## License

[Your License Here]

## Contributors

Built for TrackShift 2025

## Acknowledgments

- Inspired by Formula 1 telemetry systems
- Built with FastAPI, React, and modern Python async/await
