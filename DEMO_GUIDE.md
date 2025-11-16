# DRS-SYNC Demo Guide for TrackShift 2025

This guide walks through the demo scenarios to showcase DRS-SYNC's capabilities.

## Pre-Demo Setup

1. **Start all services** (see QUICKSTART.md)
2. **Prepare test files**:
   ```bash
   # Create a 10MB test file
   dd if=/dev/urandom of=test_10mb.bin bs=1M count=10
   ```

3. **Open the dashboard**: http://localhost:3000

## Demo 1: Baseline Burst Transfer

**Goal**: Show normal high-speed transfer

**Steps**:
1. Upload `test_10mb.bin` with High priority
2. Start transfer to `localhost:9000`
3. Point out:
   - Real-time progress bar
   - Throughput metrics
   - Chunk completion counter
   - Link being used

**Key Points**:
- "This is our baseline - clean transfer at full speed"
- "Notice the real-time metrics updating"
- "The system automatically selected the best link"

**Duration**: ~30 seconds

---

## Demo 2: Kill the Uplink - Auto-Adaptation

**Goal**: Show resilience to network degradation

**Steps**:
1. Start a new transfer (or use existing)
2. In Control Panel → Network Simulator:
   - Set Packet Loss: 70%
   - Set Latency: 100ms
   - Enable Simulator
   - Click "Apply"
3. Point out:
   - Transfer slows down
   - Retransmission counter increases
   - System adapts but continues
4. Reset simulator to show recovery

**Key Points**:
- "Watch what happens when we simulate 70% packet loss"
- "The system automatically retransmits lost chunks"
- "Transfer continues despite network issues"
- "When conditions improve, speed recovers"

**Duration**: ~1 minute

---

## Demo 3: Crash & Resume

**Goal**: Show checkpoint/resume capability

**Steps**:
1. Start a transfer
2. Let it run to ~30% completion
3. **Kill the backend server** (Ctrl+C in Terminal 2)
4. Point out: "Transfer stopped, but data is safe"
5. **Restart the server**: `python3 scripts/start_server.py`
6. Show the transfer automatically resumes
7. Point out:
   - Progress continues from checkpoint
   - No data loss
   - Time saved vs. restarting from 0%

**Key Points**:
- "What if the system crashes mid-transfer?"
- "All progress is saved in checkpoints"
- "When we restart, it picks up exactly where it left off"
- "This is critical for large telemetry files"

**Duration**: ~1.5 minutes

---

## Demo 4: Multi-Uplink Switching

**Goal**: Show automatic link switching

**Prerequisites**: Need at least 2 active network interfaces

**Steps**:
1. Show current links in Metrics Panel
2. Identify primary link (highest score)
3. Start a transfer
4. In Control Panel:
   - Select primary interface (e.g., "en0")
   - Click "Kill Link"
5. Point out:
   - System detects link failure
   - Automatically switches to secondary link
   - Transfer continues seamlessly
   - Link switch counter increments
6. Restore the link to show it switches back

**Key Points**:
- "Motorsport environments have flaky connectivity"
- "The system continuously monitors all available links"
- "When one fails, it automatically switches to the best alternative"
- "Zero downtime, zero data loss"

**Duration**: ~1 minute

---

## Demo 5: Multipath Boost (Optional/Future)

**Goal**: Show simultaneous multi-link transfer

**Note**: This is a future enhancement. For now, you can:
1. Explain the concept
2. Show how the system could send different chunks over different links
3. Demonstrate link selection logic

**Key Points**:
- "In the future, we can send chunks over multiple links simultaneously"
- "This would aggregate bandwidth for even faster transfers"
- "The chunk scheduler would distribute work across links"

---

## Demo Flow Script

For a smooth presentation, follow this order:

1. **Introduction** (30s)
   - "DRS-SYNC solves the problem of unreliable networks in motorsport"
   - Show dashboard overview

2. **Demo 1: Baseline** (30s)
   - Show normal operation

3. **Demo 2: Network Issues** (1m)
   - Show resilience

4. **Demo 3: Crash Recovery** (1.5m)
   - Show reliability

5. **Demo 4: Link Switching** (1m)
   - Show intelligence

6. **Summary** (30s)
   - Recap key features
   - Show metrics: time saved, reliability, adaptability

**Total Time**: ~5 minutes

## Talking Points

### Problem Statement
- "Motorsport telemetry generates massive amounts of data"
- "Network conditions at tracks are unpredictable"
- "We need 100% reliability even when links fail"

### Solution Highlights
- "Multi-uplink scanning finds the best path"
- "Adaptive selection switches automatically"
- "Checkpointing ensures zero data loss"
- "Real-time monitoring for visibility"

### Technical Excellence
- "Sliding window protocol for efficiency"
- "Exponential backoff for retries"
- "Interface binding for true multi-uplink"
- "WebSocket for real-time updates"

### Use Cases
- "Race telemetry during sessions"
- "Video uploads from trackside cameras"
- "LiDAR data from scanning vehicles"
- "Strategy packets to pit wall"

## Metrics to Highlight

During demos, point out:

1. **Throughput**: Real-time Mbps
2. **Reliability**: Retransmission rate
3. **Adaptability**: Link switches
4. **Efficiency**: Time saved by resume
5. **Visibility**: Real-time metrics

## Troubleshooting During Demo

If something goes wrong:

- **Transfer stops**: Check receiver is running
- **No links found**: Verify network interfaces
- **Dashboard not updating**: Check WebSocket connection
- **Simulator not working**: Verify API calls

**Recovery**: Have backup screenshots or video ready!

## Post-Demo Q&A Prep

Be ready to answer:

- "How does it compare to rsync/scp?"
  - Multi-uplink, checkpointing, real-time adaptation

- "What about security?"
  - Currently demo mode, production would add TLS/DTLS

- "Can it handle video streaming?"
  - Current: file transfer. Future: streaming support

- "What's the maximum file size?"
  - No hard limit, tested with multi-GB files

- "How does link scoring work?"
  - Weighted algorithm: throughput, RTT, loss, stability

## Success Criteria

A successful demo shows:

✅ **Reliability**: Transfer completes despite failures
✅ **Intelligence**: Automatic link selection and switching
✅ **Visibility**: Real-time metrics and progress
✅ **Resilience**: Crash recovery and resume
✅ **Performance**: Efficient use of available bandwidth

Good luck with your TrackShift 2025 presentation! 🏁

