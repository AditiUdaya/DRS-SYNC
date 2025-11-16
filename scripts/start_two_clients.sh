#!/bin/bash
# Script to start 2-client test setup

echo "🚦 Starting DRS-SYNC Two-Client Test Setup"
echo "=========================================="
echo ""

# Check if virtual environment is activated
if [ -z "$VIRTUAL_ENV" ]; then
    echo "⚠️  Virtual environment not activated. Activating..."
    source venv/bin/activate
fi

# Start receiver 1 in background
echo "📡 Starting Receiver 1 on port 9000..."
python3 -m backend.receiver 9000 > /tmp/receiver1.log 2>&1 &
RECEIVER1_PID=$!
echo "   Receiver 1 PID: $RECEIVER1_PID"

# Wait a bit
sleep 1

# Start receiver 2 in background
echo "📡 Starting Receiver 2 on port 9001..."
python3 -m backend.receiver 9001 > /tmp/receiver2.log 2>&1 &
RECEIVER2_PID=$!
echo "   Receiver 2 PID: $RECEIVER2_PID"

# Wait a bit
sleep 1

# Start backend server in background
echo "🚀 Starting Backend Server on port 8080..."
python3 scripts/start_server.py > /tmp/backend.log 2>&1 &
BACKEND_PID=$!
echo "   Backend PID: $BACKEND_PID"

# Wait for services to start
echo ""
echo "⏳ Waiting for services to start..."
sleep 3

echo ""
echo "✅ All services started!"
echo ""
echo "📊 Logs:"
echo "   Receiver 1: tail -f /tmp/receiver1.log"
echo "   Receiver 2: tail -f /tmp/receiver2.log"
echo "   Backend:    tail -f /tmp/backend.log"
echo ""
echo "🧪 Run test:"
echo "   python3 scripts/test_two_clients.py"
echo ""
echo "🛑 To stop all services:"
echo "   kill $RECEIVER1_PID $RECEIVER2_PID $BACKEND_PID"
echo ""

# Keep script running
wait

