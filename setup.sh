#!/bin/bash
# DRS-SYNC Setup Script

set -e

echo "🚦 DRS-SYNC Setup"
echo "=================="
echo ""

# Check Python
if ! command -v python3 &> /dev/null; then
    echo "❌ Error: python3 not found. Please install Python 3.8+"
    exit 1
fi

PYTHON_VERSION=$(python3 --version | cut -d' ' -f2 | cut -d'.' -f1,2)
echo "✓ Python version: $PYTHON_VERSION"

# Check Node.js
if ! command -v node &> /dev/null; then
    echo "❌ Error: node not found. Please install Node.js 16+"
    exit 1
fi

NODE_VERSION=$(node --version)
echo "✓ Node.js version: $NODE_VERSION"

# Create virtual environment
echo ""
echo "Creating Python virtual environment..."
if [ ! -d "venv" ]; then
    python3 -m venv venv
    echo "✓ Virtual environment created"
else
    echo "✓ Virtual environment already exists"
fi

# Activate virtual environment
source venv/bin/activate

# Install Python dependencies
echo ""
echo "Installing Python dependencies..."
pip install -q --upgrade pip
pip install -q -r requirements.txt
echo "✓ Python dependencies installed"

# Install Node.js dependencies
echo ""
echo "Installing Node.js dependencies..."
cd frontend
if [ ! -d "node_modules" ]; then
    npm install --silent
    echo "✓ Node.js dependencies installed"
else
    echo "✓ Node.js dependencies already installed"
fi
cd ..

# Create necessary directories
echo ""
echo "Creating data directories..."
mkdir -p data/manifests data/cache uploads received
echo "✓ Directories created"

# Make scripts executable
echo ""
echo "Making scripts executable..."
chmod +x scripts/*.sh
chmod +x cli.py
chmod +x setup.sh
echo "✓ Scripts are executable"

echo ""
echo "✅ Setup complete!"
echo ""
echo "Next steps:"
echo "  1. Start the receiver:    python3 -m backend.receiver"
echo "  2. Start the server:      python3 scripts/start_server.py"
echo "  3. Start the frontend:    cd frontend && npm start"
echo ""
echo "Or use the demo script:"
echo "  ./scripts/start_demo.sh"
echo ""
echo "See QUICKSTART.md for detailed instructions."

