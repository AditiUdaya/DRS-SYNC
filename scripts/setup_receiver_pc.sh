#!/bin/bash
# Setup script for Receiver PC (PC 1)

echo "🚦 DRS-SYNC Receiver PC Setup"
echo "=============================="
echo ""

# Check if virtual environment exists
if [ -d "venv" ]; then
    echo "📦 Activating virtual environment..."
    source venv/bin/activate
else
    echo "⚠️  No virtual environment found. Using system Python."
    echo "   Consider creating one: python3 -m venv venv"
    echo ""
fi

# Check if port is provided as argument
PORT=${1:-9000}

echo "🔍 Finding network interfaces..."
echo ""
python3 scripts/find_pc_ip.py

echo ""
echo "📡 Starting receiver on port $PORT..."
echo ""

# Start receiver
python3 -m backend.receiver $PORT

