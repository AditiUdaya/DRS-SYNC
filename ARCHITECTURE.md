# DRS-SYNC Architecture

## System Overview

DRS-SYNC is a multi-layered system designed for reliable file transfer over unstable networks. The architecture follows a modular design with clear separation of concerns.

## Core Components

### 1. Interface Scanner (`backend/interface_scanner.py`)

**Purpose**: Continuously monitors and evaluates all available network interfaces.

**Key Features**:
- Enumerates active network interfaces
- Measures throughput, RTT, packet loss, jitter
- Calculates stability scores
- Ranks interfaces by composite score
- Runs continuous background scanning

**Algorithm**:
1. Bind UDP socket to each interface
2. Send test packets to known host (8.8.8.8)
3. Measure response times and packet loss
4. Calculate normalized metrics
5. Compute weighted composite score

### 2. Chunk Manager (`backend/chunk_manager.py`)

**Purpose**: Manages file chunking, manifests, and checkpointing.

**Key Features**:
- Splits files into fixed-size chunks (64KB default)
- Calculates xxHash for each chunk
- Maintains per-file manifests (JSON)
- Tracks chunk status (pending, in_flight, acked, failed)
- Enables resume from any checkpoint

**Manifest Structure**:
- File metadata (ID, path, size, SHA-256 hash)
- Chunk list with offsets, sizes, hashes
- Transfer state (bytes transferred, completion status)
- Timestamps for resume calculations

### 3. Transfer Engine (`backend/transfer_engine.py`)

**Purpose**: Orchestrates reliable file transfer with adaptive link selection.

**Key Features**:
- Sliding window protocol (10 chunks in flight)
- Automatic retransmission on timeout
- Exponential backoff for retries
- Interface binding for multi-uplink
- Real-time progress tracking

**Transfer Flow**:
1. Load or create manifest
2. Get best available link from scanner
3. Bind socket to selected interface
4. Send chunks up to window size
5. Monitor for ACKs and timeouts
6. Retransmit failed chunks
7. Switch links if current degrades
8. Save checkpoint on completion

### 4. Network Simulator (`backend/network_simulator.py`)

**Purpose**: Simulates network conditions for testing and demos.

**Key Features**:
- Configurable packet loss (0-100%)
- Variable latency (ms)
- Jitter simulation
- Link kill/restore
- Per-interface or global configuration

**Usage**:
- Wrap send operations
- Apply delays and drops
- Enable realistic testing scenarios

### 5. API Server (`backend/api.py`)

**Purpose**: Provides REST API and WebSocket interface.

**REST Endpoints**:
- File management (upload, list, status)
- Transfer control (start, pause, resume, cancel)
- Link monitoring
- Simulator configuration

**WebSocket**:
- Real-time link metrics broadcast
- Transfer progress updates
- Event streaming

### 6. React Dashboard (`frontend/`)

**Purpose**: Real-time visualization and control interface.

**Components**:
- **FileQueue**: Upload and manage files
- **TransferCanvas**: Visualize active transfers
- **MetricsPanel**: Live link performance graphs
- **ControlPanel**: Simulator controls

**Real-time Updates**:
- WebSocket connection for live data
- Auto-refresh on state changes
- Event log for debugging

## Data Flow

### Upload Flow

```
User → Frontend → REST API → Chunk Manager
                              ↓
                         Create Manifest
                              ↓
                         Store in Queue
```

### Transfer Flow

```
Transfer Engine → Get Best Link → Bind Socket
                      ↓
                 Load Chunks
                      ↓
              Sliding Window Send
                      ↓
         [Network Simulator] → UDP Socket
                      ↓
                 Receiver
```

### Resume Flow

```
Crash/Disconnect → Manifest Persisted
                      ↓
              Restart Engine
                      ↓
              Load Manifest
                      ↓
         Identify Pending Chunks
                      ↓
              Resume Transfer
```

## Link Selection Algorithm

### Scoring Formula

```
link_score = Σ(weight_i × metric_i_normalized)
```

### Metrics

1. **Throughput** (40% weight)
   - Measured: Bytes sent per second
   - Normalized: 0-100 Mbps → 0-1

2. **RTT** (30% weight)
   - Measured: Round-trip time in ms
   - Normalized: 0-200ms → 1-0 (inverse)

3. **Packet Loss** (20% weight)
   - Measured: Lost packets / sent packets
   - Normalized: 1 - loss_rate

4. **Stability** (10% weight)
   - Measured: Jitter + loss variance
   - Normalized: 1 - (jitter/100) - loss

### Selection Logic

1. Scan all interfaces every 5 seconds
2. Filter interfaces with score > 0.1
3. Select highest scoring interface
4. Switch if current link score drops below threshold
5. Prefer stable links for long transfers

## Reliability Mechanisms

### Sliding Window

- Maintains N chunks in flight (default: 10)
- Prevents overwhelming slow links
- Enables pipelining for throughput

### Retransmission

- Timeout-based detection
- Exponential backoff: `delay = base × 2^retry_count`
- Maximum 3 retries per chunk
- Failed chunks marked for manual retry

### Checkpointing

- Manifest saved after each chunk ACK
- Atomic updates prevent corruption
- Resume from last ACKed chunk
- No data loss on crash

### Hash Verification

- xxHash64 for chunk integrity (fast)
- SHA-256 for file integrity (final)
- Automatic retransmission on mismatch

## Network Interface Binding

### Linux

```python
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE, interface.encode())
```

### macOS/Windows

- Uses routing table manipulation
- May require elevated privileges
- Fallback to default routing

## Performance Optimizations

1. **Async I/O**: All network operations are async
2. **Chunked Reads**: Stream large files in chunks
3. **Parallel Scanning**: Test interfaces concurrently
4. **Efficient Hashing**: xxHash for speed, SHA-256 for security
5. **WebSocket Batching**: Group updates to reduce overhead

## Scalability Considerations

### Current Limitations

- Single-threaded async (sufficient for demo)
- In-memory transfer state
- Local file storage

### Future Enhancements

- Distributed transfer nodes
- Redis for shared state
- S3/cloud storage integration
- Multipath TCP support
- Bandwidth aggregation

## Security Considerations

### Current Implementation

- No authentication (demo only)
- No encryption (UDP plaintext)
- Local file access

### Production Requirements

- TLS/DTLS for encrypted transfer
- API authentication (JWT/OAuth)
- File access controls
- Audit logging

## Testing Strategy

### Unit Tests

- Chunk manager operations
- Link scoring algorithm
- Manifest serialization

### Integration Tests

- End-to-end transfers
- Resume scenarios
- Link switching

### Demo Scenarios

- Baseline performance
- Network degradation
- Crash recovery
- Multi-uplink switching

## Deployment

### Development

```bash
python3 scripts/start_server.py
cd frontend && npm start
```

### Production

- Use gunicorn/uvicorn workers
- Nginx reverse proxy
- React build served statically
- Systemd service for receiver
- Docker containers for isolation

## Monitoring

### Metrics Collected

- Transfer throughput (Mbps)
- Chunk retransmission rate
- Link switch frequency
- Average RTT per link
- Packet loss per link
- Time saved by resume

### Logging

- Transfer events (start, pause, resume, complete)
- Link state changes
- Error conditions
- Performance metrics

## Future Work

1. **Multipath Transfer**: Send chunks over multiple links simultaneously
2. **Adaptive Chunking**: Dynamic chunk size based on link quality
3. **Compression**: On-the-fly compression for bandwidth savings
4. **Encryption**: End-to-end encryption for sensitive data
5. **Cloud Integration**: Direct upload to S3/GCS
6. **Mobile App**: iOS/Android for field use
7. **ML-Based Link Prediction**: Predict link quality before switching

