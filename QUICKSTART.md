# DRS-SYNC Quick Start Guide

Get DRS-SYNC running in 5 minutes!

> **Testing between two PCs?** See [TWO_PC_SETUP.md](TWO_PC_SETUP.md) for instructions on testing file transfers between two separate computers.

## Prerequisites

- Python 3.8 or higher
- Node.js 16+ and npm
- At least one active network interface (WiFi or Ethernet)

## Step 1: Install Dependencies

### Backend (Python)

```bash
# Create virtual environment
python3 -m venv venv

# Activate it
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt
```

### Frontend (React)

```bash
cd frontend
npm install
cd ..
```

## Step 2: Start the Services

You'll need **3 terminal windows**:

### Terminal 1: Receiver

```bash
source venv/bin/activate  # If not already activated
python3 -m backend.receiver
```

You should see:
```
Receiver listening on port 9000
Output directory: /path/to/DRS-SYNC/received
```

### Terminal 2: Backend Server

```bash
source venv/bin/activate
python3 scripts/start_server.py
```

You should see:
```
🚦 Starting DRS-SYNC Server...
API: http://0.0.0.0:8080
WebSocket: ws://0.0.0.0:8080/ws
```

### Terminal 3: Frontend

```bash
cd frontend
npm start
```

The React app will open automatically at http://localhost:3000

## Step 3: Test the System

### Option A: Using the Web Dashboard

1. Open http://localhost:3000 in your browser
2. Click "Choose File" and select any file
3. Set priority (High/Standard/Background)
4. Click "Upload File"
5. Set destination (default: localhost:9000)
6. Click "Start" on the uploaded file
7. Watch the transfer in real-time!

### Option B: Using the CLI

```bash
# Scan network interfaces
python3 cli.py scan

# Upload a file
python3 cli.py upload /path/to/your/file.bin --priority high

# Check status
python3 cli.py status

# Start transfer (use file_id from status)
python3 cli.py transfer <file_id> --host localhost --port 9000
```

## Step 4: Try the Demos

Run the demo scenarios:

```bash
python3 scripts/demo.py
```

Or use the all-in-one demo script:

```bash
chmod +x scripts/start_demo.sh
./scripts/start_demo.sh
```

## Demo Scenarios

### Demo 1: Baseline Transfer
- Upload a file and watch it transfer at full speed

### Demo 2: Network Degradation
1. Start a transfer
2. In the dashboard, go to Control Panel
3. Set Packet Loss to 70%
4. Enable Simulator
5. Click "Apply"
6. Watch the transfer slow down and adapt

### Demo 3: Crash & Resume
1. Start a transfer
2. Kill the backend server (Ctrl+C)
3. Restart the server
4. The transfer resumes from the last checkpoint!

### Demo 4: Link Switching
1. Start a transfer
2. In Control Panel, select your WiFi interface
3. Click "Kill Link"
4. Watch the system automatically switch to another interface

## Troubleshooting

### "No network interfaces found"
- Make sure you have WiFi or Ethernet connected
- On Linux, you may need `sudo` for interface binding
- Check interfaces: `ip link show` (Linux) or `ifconfig` (macOS)

### "Connection refused" errors
- Make sure all 3 services are running
- Check that ports 8080 (API) and 9000 (receiver) are not in use
- Verify firewall settings

### Frontend can't connect to backend
- Check that backend is running on port 8080
- Verify CORS settings in `backend/api.py`
- Check browser console for errors

### Transfer not starting
- Ensure receiver is running on port 9000
- Check that destination host/port are correct
- Verify network connectivity

## Next Steps

- Read the full [README.md](README.md) for detailed documentation
- Check [ARCHITECTURE.md](ARCHITECTURE.md) for system design
- See [API.md](API.md) for API reference
- Explore the code in `backend/` and `frontend/src/`

## Getting Help

- Check the logs in each terminal window
- Review error messages in the browser console
- Verify all dependencies are installed correctly
- Ensure Python and Node.js versions are compatible

## Production Deployment

For production use, consider:

1. **Security**: Add authentication and encryption
2. **Scalability**: Use multiple workers and load balancing
3. **Monitoring**: Add logging and metrics collection
4. **Storage**: Use cloud storage (S3, GCS) instead of local files
5. **Deployment**: Use Docker containers and orchestration

See the main README for more details.

