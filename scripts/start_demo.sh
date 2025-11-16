#!/bin/bash
# Start DRS-SYNC demo environment

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_DIR"

echo "🚦 Starting DRS-SYNC Demo Environment"
echo "======================================"
echo ""

# Check Python
if ! command -v python3 &> /dev/null; then
    echo "Error: python3 not found"
    exit 1
fi

# Check if virtual environment exists
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
fi

# Activate virtual environment
source venv/bin/activate

# Install dependencies
echo "Installing dependencies..."
pip install -q -r requirements.txt

# Create necessary directories
mkdir -p data/manifests data/cache uploads received

# Start receiver in background
echo "Starting receiver on port 9000..."
python3 -m backend.receiver &
RECEIVER_PID=$!

# Wait a moment
sleep 2

# Start server
echo "Starting DRS-SYNC server on port 8080..."
echo ""
echo "Dashboard: http://localhost:3000 (if frontend is running)"
echo "API: http://localhost:8080"
echo ""
echo "Press Ctrl+C to stop all services"
echo ""

# Trap Ctrl+C to kill receiver
trap "kill $RECEIVER_PID 2>/dev/null; exit" INT TERM

# Start server
python3 scripts/start_server.py

# Cleanup
kill $RECEIVER_PID 2>/dev/null || true

