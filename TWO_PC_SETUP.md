# Two-PC Testing Setup Guide

This guide will help you test DRS-SYNC file transfers between two separate computers on the same network.

## Overview

- **PC 1 (Receiver)**: Receives file chunks via UDP
- **PC 2 (Sender)**: Runs the backend server, uploads files, and sends them to PC 1

## Prerequisites

1. **Both PCs on the same network** (same WiFi or LAN)
2. **Python 3.8+** installed on both PCs
3. **DRS-SYNC codebase** cloned/copied to both PCs (or at least on sender PC)
4. **Firewall configured**:
   - Receiver PC: Allow UDP on port 9000 (or your chosen port)
   - Sender PC: Allow TCP on port 8080 (for API, optional if using CLI only)

## Step 1: Find IP Addresses

### On Both PCs

Run the IP discovery script:

```bash
python3 scripts/find_pc_ip.py
```

This will show:
- All active network interfaces
- Their IP addresses
- Recommended interface for transfers

**Note the IP address** of the recommended interface on PC 1 (Receiver).

## Step 2: Setup PC 1 (Receiver)

### Option A: Using the Setup Script

```bash
# Make script executable (first time only)
chmod +x scripts/setup_receiver_pc.sh

# Run the setup script
./scripts/setup_receiver_pc.sh
```

### Option B: Manual Setup

1. **Activate virtual environment** (if using one):
   ```bash
   source venv/bin/activate  # Linux/macOS
   # or
   venv\Scripts\activate     # Windows
   ```

2. **Start the receiver**:
   ```bash
   python3 -m backend.receiver 9000
   ```

3. **Note the displayed information**:
   - The receiver will show its IP address
   - Example: `192.168.1.100:9000`
   - **Share this IP:port with the sender PC**

The receiver will display:
```
📡 DRS-SYNC Receiver Started
✅ Listening on port: 9000
💡 Share this IP address with the sender PC:
   192.168.1.100:9000
```

**Keep this terminal open** - the receiver must stay running.

## Step 3: Setup PC 2 (Sender)

### Option A: Using the Setup Script

```bash
# Make script executable (first time only)
chmod +x scripts/setup_sender_pc.sh

# Run the setup script
./scripts/setup_sender_pc.sh
```

### Option B: Manual Setup

1. **Activate virtual environment**:
   ```bash
   source venv/bin/activate  # Linux/macOS
   # or
   venv\Scripts\activate     # Windows
   ```

2. **Start the backend server**:
   ```bash
   python3 scripts/start_server.py
   ```

3. **Start the dashboard** (optional, in another terminal):
   ```bash
   cd frontend
   npm start
   ```

4. **Note**: The backend will be available at `http://localhost:8080`

## Step 4: Test the Transfer

### Option A: Using the Test Script (Recommended)

On PC 2 (Sender), run:

```bash
python3 scripts/test_two_pc_transfer.py <RECEIVER_IP> 9000
```

Replace `<RECEIVER_IP>` with the IP address from PC 1.

Example:
```bash
python3 scripts/test_two_pc_transfer.py 192.168.1.100 9000
```

### Option B: Using the Dashboard

1. Open the dashboard in a browser: `http://localhost:8080` (or the sender PC's IP)
2. Upload a file using the "Upload File" button
3. Click "Start Transfer"
4. Enter the receiver IP and port:
   - **Destination Host**: `192.168.1.100` (PC 1's IP)
   - **Destination Port**: `9000`
5. Click "Start" and monitor the progress

### Option C: Using the API

```bash
# 1. Upload a file
curl -X POST http://localhost:8080/api/files/upload \
  -F "file=@test_file.txt" \
  -F "priority=high"

# Note the file_id from the response

# 2. Start transfer
curl -X POST http://localhost:8080/api/files/<file_id>/transfer \
  -H "Content-Type: application/json" \
  -d '{
    "file_id": "<file_id>",
    "destination_host": "192.168.1.100",
    "destination_port": 9000
  }'
```

## Step 5: Verify Transfer

### On PC 1 (Receiver)

You should see messages like:
```
Received chunk 0 from <file_id> (65536 bytes) from 192.168.1.50:54321
Received chunk 1 from <file_id> (65536 bytes) from 192.168.1.50:54321
...
```

### On PC 2 (Sender)

You should see:
```
✅ Using best link: en0 (192.168.1.50) - Score: 74.00%
📤 Sent 10 chunks for <file_id> to 192.168.1.100:9000
✅ ACK received for chunk 0 of <file_id>
...
```

### Check Received File

On PC 1, check the `received/` directory:
```bash
ls -lh received/
```

The file should be reconstructed there.

## Troubleshooting

### "No network links available"

**Problem**: Sender can't find network interfaces

**Solution**:
- Make sure network adapter is enabled
- Check firewall settings
- Run `python3 scripts/find_pc_ip.py` to verify interfaces

### "Connection refused" or no chunks received

**Problem**: Network connectivity issue

**Solutions**:
1. **Verify both PCs are on the same network**:
   ```bash
   # On PC 2, ping PC 1
   ping 192.168.1.100
   ```

2. **Check firewall**:
   - Receiver PC: Allow UDP port 9000
   - Sender PC: Allow outbound UDP

3. **Verify receiver is running**:
   - Check PC 1 terminal for receiver output
   - Try restarting receiver

4. **Check IP address**:
   - Make sure you're using the correct IP from PC 1
   - Run `python3 scripts/find_pc_ip.py` again to confirm

### "Packet dropped by simulator"

**Problem**: Network simulator is enabled with packet loss

**Solution**: Disable the simulator:
```bash
curl -X POST http://localhost:8080/api/simulator/config \
  -H "Content-Type: application/json" \
  -d '{"enabled": false, "packet_loss": 0.0}'
```

Or use the dashboard to disable it in the Control Panel.

### Transfer stuck at 0%

**Problem**: No ACKs being received

**Solutions**:
1. Make sure receiver is running on PC 1
2. Verify IP address and port are correct
3. Check firewall allows UDP traffic
4. Disable network simulator if enabled
5. Check receiver logs for incoming chunks

### Port already in use

**Problem**: Another process is using the port

**Solution**:
```bash
# Find process using port 9000
lsof -i:9000  # Linux/macOS
netstat -ano | findstr :9000  # Windows

# Kill the process or use a different port
python3 -m backend.receiver 9001
```

## Network Interface Types

DRS-SYNC automatically detects and can use:

- **Ethernet** (`eth0`, `en0`) - Wired connection
- **WiFi** (`wlan0`, `en1`) - Wireless connection
- **Cellular/Modem** (`ppp0`, `wwan0`) - Mobile data
- **USB Adapters** (`usb0`) - USB network adapters
- **VPN Tunnels** (`tun0`, `tap0`) - VPN connections

The system automatically selects the best interface based on performance metrics.

## Advanced: Testing Different Interfaces

To test transfers over a specific interface:

1. **On PC 1**: Note the IP of the interface you want to test
2. **On PC 2**: Use that IP when starting the transfer
3. The system will automatically route through the best interface

## Next Steps

- Test with different file sizes
- Test with network simulator enabled (packet loss, latency)
- Test with multiple simultaneous transfers
- Test interface switching (disconnect WiFi, connect Ethernet)

## Quick Reference

**PC 1 (Receiver)**:
```bash
python3 -m backend.receiver 9000
# Note the IP address shown
```

**PC 2 (Sender)**:
```bash
# Start backend
python3 scripts/start_server.py

# Test transfer
python3 scripts/test_two_pc_transfer.py <PC1_IP> 9000
```

**Find IP addresses**:
```bash
python3 scripts/find_pc_ip.py
```

