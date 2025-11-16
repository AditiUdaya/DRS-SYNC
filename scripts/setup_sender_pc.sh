#!/bin/bash
# Setup script for Sender PC (PC 2)

echo "🚦 DRS-SYNC Sender PC Setup"
echo "==========================="
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

echo "🔍 Finding network interfaces..."
echo ""
python3 scripts/find_pc_ip.py

echo ""
echo "📡 Starting backend server..."
echo "   API will be available at: http://localhost:8080"
echo "   WebSocket: ws://localhost:8080/ws"
echo ""
echo "💡 In another terminal, you can:"
echo "   1. Start the dashboard: cd frontend && npm start"
echo "   2. Or use the test script: python3 scripts/test_two_pc_transfer.py <RECEIVER_IP> 9000"
echo ""
echo "Press Ctrl+C to stop the server"
echo ""

# Start backend server
python3 scripts/start_server.py

