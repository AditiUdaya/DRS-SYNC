# DRS-SYNC API Documentation

## Base URL

```
http://localhost:8080/api
```

## Authentication

Currently, no authentication is required (demo mode). Production deployments should implement JWT or OAuth2.

## Endpoints

### Network Links

#### GET /api/links

Get all network interfaces and their current metrics.

**Response:**
```json
{
  "links": [
    {
      "interface": "en0",
      "ip_address": "192.168.1.100",
      "throughput_mbps": 45.2,
      "rtt_ms": 12.5,
      "packet_loss": 0.01,
      "jitter_ms": 2.3,
      "stability_score": 0.95,
      "link_score": 0.87,
      "is_active": true
    }
  ]
}
```

#### GET /api/links/best

Get the currently best performing network link.

**Response:**
```json
{
  "interface": "en0",
  "ip_address": "192.168.1.100",
  "throughput_mbps": 45.2,
  "rtt_ms": 12.5,
  "packet_loss": 0.01,
  "link_score": 0.87
}
```

### File Management

#### POST /api/files/upload

Upload a file to the transfer queue.

**Request:**
- Method: `POST`
- Content-Type: `multipart/form-data`
- Body:
  - `file`: File to upload
  - `priority`: `high`, `standard`, or `background` (optional, default: `standard`)

**Response:**
```json
{
  "file_id": "550e8400-e29b-41d4-a716-446655440000",
  "filename": "telemetry.bin",
  "size": 10485760,
  "priority": "high",
  "total_chunks": 160,
  "status": "queued"
}
```

#### GET /api/files

List all files in the system.

**Response:**
```json
{
  "files": [
    {
      "file_id": "550e8400-e29b-41d4-a716-446655440000",
      "filename": "telemetry.bin",
      "size": 10485760,
      "priority": "high",
      "progress": {
        "progress": 0.45,
        "bytes_transferred": 4718592,
        "bytes_total": 10485760,
        "chunks_complete": 72,
        "chunks_total": 160
      },
      "status": "active",
      "is_paused": false
    }
  ]
}
```

#### GET /api/files/{file_id}

Get detailed status of a specific file transfer.

**Response:**
```json
{
  "file_id": "550e8400-e29b-41d4-a716-446655440000",
  "filename": "telemetry.bin",
  "size": 10485760,
  "priority": "high",
  "progress": {
    "progress": 0.45,
    "bytes_transferred": 4718592,
    "bytes_total": 10485760,
    "chunks_complete": 72,
    "chunks_total": 160
  },
  "status": {
    "start_time": "2025-01-01T12:00:00",
    "bytes_sent": 5242880,
    "chunks_sent": 80,
    "chunks_acked": 72,
    "retransmissions": 3,
    "link_switches": 1,
    "current_link": "en0",
    "is_active": true,
    "is_paused": false,
    "throughput_mbps": 12.5
  }
}
```

### Transfer Control

#### POST /api/files/{file_id}/transfer

Start transferring a file.

**Request:**
```json
{
  "file_id": "550e8400-e29b-41d4-a716-446655440000",
  "destination_host": "192.168.1.200",
  "destination_port": 9000
}
```

**Response:**
```json
{
  "status": "started",
  "file_id": "550e8400-e29b-41d4-a716-446655440000"
}
```

#### POST /api/files/{file_id}/pause

Pause an active transfer.

**Response:**
```json
{
  "status": "paused",
  "file_id": "550e8400-e29b-41d4-a716-446655440000"
}
```

#### POST /api/files/{file_id}/resume

Resume a paused transfer.

**Response:**
```json
{
  "status": "resumed",
  "file_id": "550e8400-e29b-41d4-a716-446655440000"
}
```

#### POST /api/files/{file_id}/cancel

Cancel a transfer (cannot be resumed).

**Response:**
```json
{
  "status": "cancelled",
  "file_id": "550e8400-e29b-41d4-a716-446655440000"
}
```

#### PUT /api/files/{file_id}/priority

Update the priority of a file.

**Request:**
```json
{
  "priority": "high"
}
```

**Response:**
```json
{
  "status": "updated",
  "file_id": "550e8400-e29b-41d4-a716-446655440000",
  "priority": "high"
}
```

### Network Simulator

#### POST /api/simulator/config

Configure the network simulator.

**Request:**
```json
{
  "packet_loss": 0.7,
  "latency_ms": 100,
  "jitter_ms": 20,
  "enabled": true,
  "kill_link": false,
  "interface": "en0"
}
```

**Response:**
```json
{
  "status": "configured",
  "config": {
    "packet_loss": 0.7,
    "latency_ms": 100,
    "jitter_ms": 20,
    "enabled": true,
    "kill_link": false,
    "interface": "en0"
  }
}
```

#### POST /api/simulator/kill-link

Simulate killing a network link.

**Query Parameters:**
- `interface` (optional): Specific interface to kill, or all if omitted

**Response:**
```json
{
  "status": "link_killed",
  "interface": "en0"
}
```

#### POST /api/simulator/restore-link

Restore a killed link.

**Query Parameters:**
- `interface` (optional): Specific interface to restore, or all if omitted

**Response:**
```json
{
  "status": "link_restored",
  "interface": "en0"
}
```

#### POST /api/simulator/reset

Reset simulator configuration.

**Query Parameters:**
- `interface` (optional): Specific interface to reset, or all if omitted

**Response:**
```json
{
  "status": "reset",
  "interface": "en0"
}
```

## WebSocket API

### Connection

Connect to: `ws://localhost:8080/ws`

### Message Types

#### link_metrics

Periodic updates of network link metrics (every 2 seconds).

```json
{
  "type": "link_metrics",
  "links": [
    {
      "interface": "en0",
      "ip_address": "192.168.1.100",
      "throughput_mbps": 45.2,
      "rtt_ms": 12.5,
      "packet_loss": 0.01,
      "jitter_ms": 2.3,
      "stability_score": 0.95,
      "link_score": 0.87,
      "is_active": true
    }
  ],
  "timestamp": "2025-01-01T12:00:00.123456"
}
```

#### transfer_progress

Real-time transfer progress updates.

```json
{
  "type": "transfer_progress",
  "file_id": "550e8400-e29b-41d4-a716-446655440000",
  "progress": {
    "progress": 0.45,
    "bytes_transferred": 4718592,
    "bytes_total": 10485760,
    "chunks_complete": 72,
    "chunks_total": 160
  },
  "stats": {
    "start_time": "2025-01-01T12:00:00",
    "bytes_sent": 5242880,
    "chunks_sent": 80,
    "chunks_acked": 72,
    "retransmissions": 3,
    "link_switches": 1,
    "current_link": "en0",
    "throughput_mbps": 12.5
  }
}
```

## Error Responses

All endpoints may return error responses in the following format:

```json
{
  "detail": "Error message description"
}
```

Common HTTP status codes:
- `200`: Success
- `400`: Bad Request (invalid parameters)
- `404`: Not Found (file/link not found)
- `500`: Internal Server Error

## Rate Limiting

Currently, no rate limiting is implemented. Production deployments should add rate limiting to prevent abuse.

## Examples

### Upload and Transfer a File

```bash
# Upload file
curl -X POST http://localhost:8080/api/files/upload \
  -F "file=@telemetry.bin" \
  -F "priority=high"

# Response: {"file_id": "550e8400-...", ...}

# Start transfer
curl -X POST http://localhost:8080/api/files/550e8400-.../transfer \
  -H "Content-Type: application/json" \
  -d '{
    "file_id": "550e8400-...",
    "destination_host": "192.168.1.200",
    "destination_port": 9000
  }'
```

### Monitor Links

```bash
# Get all links
curl http://localhost:8080/api/links

# Get best link
curl http://localhost:8080/api/links/best
```

### Simulate Network Conditions

```bash
# Enable 50% packet loss
curl -X POST http://localhost:8080/api/simulator/config \
  -H "Content-Type: application/json" \
  -d '{
    "packet_loss": 0.5,
    "enabled": true
  }'

# Kill a specific link
curl -X POST "http://localhost:8080/api/simulator/kill-link?interface=en0"

# Restore link
curl -X POST "http://localhost:8080/api/simulator/restore-link?interface=en0"
```

