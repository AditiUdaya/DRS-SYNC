# UDP Reliable File Transfer (Selective Repeat ARQ)

A custom reliable file transfer protocol built over UDP using Selective Repeat ARQ.  
Implements reliability, ordered delivery, retransmissions, RTT estimation, and sliding-window throughput control entirely in user space.

---

## Description

This project provides a TCP-like reliability layer on top of UDP. It supports reliable transmission of arbitrarily large files by splitting them into fixed-size chunks, transmitting them with a Selective Repeat window, handling out-of-order data, and retransmitting lost packets based on timeouts.

The receiver reconstructs the entire file exactly and stores it locally.

---

## Features

| Category           | Details |
|--------------------|---------|
| Reliability        | Selective Repeat ARQ, independent per-packet ACKs, retransmissions |
| Flow Control       | Sliding Window (W = 32), per-packet timers |
| File Handling      | Chunk-based transfer (up to 64 KB per DATA packet) |
| Buffering          | Out-of-order storage on receiver until contiguous write |
| Metrics            | ACK count, retransmission count, bytes sent, RTT estimate, window utilization |
| Protocol Frames    | META, DATA, ACK |

---

## Project Structure
drogon_test/

│

├── reliable_packet.hpp          # Packet header format

├── udp_sender_sr.cpp            # Sender with Selective Repeat and metrics

├── udp_receiver_sr.cpp          # Receiver with buffering and reassembly

├── CMakeLists.txt               # Build configuration

└── build/                       # Output binaries and test files

---

## Build Instructions

Run the following commands inside the project root:

```bash
mkdir build
cd build
cmake ..
cmake --build . -j4
````

This produces:

  * `udp_sender_sr.exe`
  * `udp_receiver_sr.exe`

### Generate Test File

Create a 1 MB file in the `build` directory:

```bash
dd if=/dev/zero of=bigfile.txt bs=1M count=1
```

Verify:

```bash
ls -lh bigfile.txt
```

Expected size:

```
1.0M bigfile.txt
```

-----

## How to Run

### 1\. Start the Receiver

```bash
cd build
./udp_receiver_sr.exe
```

Typical output:

```
[receiver] listening on port 5000
[receiver] META: file_size=1048576 chunks=17
```

### 2\. Run the Sender

```bash
cd build
./udp_sender_sr.exe bigfile.txt
```

Sender output includes:

  * Packet transmissions
  * ACK receptions
  * Retransmissions
  * Periodic metrics such as:
    `[metrics] acks=17 sent_bytes=2033730 retrans=16 window_util=100% srtt_ms=1.2`

After completion, the receiver writes the file as:

`received_bigfile.txt`

-----

## Protocol Overview

### Packet Header Definition

| Field | Type | Description |
|:---|:---|:---|
| type | uint32 | 1 = DATA, 2 = ACK, 3 = META |
| id | uint32 | Chunk index |
| length | uint32 | Payload size in bytes |
| file\_size | uint64 | Used only for META frame |

### Frame Types

| Frame | Purpose |
|:---|:---|
| **META** | Announces file size and total number of chunks |
| **DATA** | Carries file chunk payloads |
| **ACK** | Acknowledges specific chunk IDs |

-----

### Sender Logic

  * Reads file and splits into 64 KB chunks.
  * Sends META frame.
  * Maintains sliding window of unacknowledged packets.
  * Retransmits timed-out packets.
  * Updates SRTT using exponential smoothing.
  * Completes once all chunks are acknowledged.

### Receiver Logic

  * Receives META and initializes buffers.
  * Stores DATA packets in out-of-order map.
  * Sends ACK for each received chunk.
  * Writes chunks to disk in order once contiguous.

-----

## Why UDP Instead of TCP

| Reason | Explanation |
|:---|:---|
| **Full Control** | User controls retransmissions, timers, window size, and pacing. |
| **Experimentation** | Enables testing custom reliable transport mechanisms. |
| **Message Boundaries** | UDP preserves packet boundaries; TCP requires manual framing. |
| **Avoiding TCP Behaviors** | No slow start, congestion control, or head-of-line blocking unless intentionally implemented. |
| **Education** | Demonstrates how TCP-like reliability can be built from scratch. |

By building Selective Repeat ARQ on UDP, this project provides a transparent, extendable, and fully customizable reliability layer suitable for experimentation and performance tuning.

```
```
