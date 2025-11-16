"""Configuration settings for DRS-SYNC"""
import os
from pathlib import Path

# Base paths
BASE_DIR = Path(__file__).parent.parent
DATA_DIR = BASE_DIR / "data"
MANIFEST_DIR = DATA_DIR / "manifests"
CACHE_DIR = DATA_DIR / "cache"
UPLOAD_DIR = BASE_DIR / "uploads"

# Create directories
for dir_path in [DATA_DIR, MANIFEST_DIR, CACHE_DIR, UPLOAD_DIR]:
    dir_path.mkdir(parents=True, exist_ok=True)

# Transfer settings
CHUNK_SIZE = 64 * 1024  # 64KB chunks
SLIDING_WINDOW_SIZE = 10  # Number of chunks in flight
MAX_RETRIES = 3
RETRY_DELAY_BASE = 1.0  # seconds
ADAPTIVE_RTT_ALPHA = 0.125  # EWMA smoothing factor

# Link scoring weights
SCORE_WEIGHTS = {
    "throughput": 0.4,
    "rtt": 0.3,
    "loss": 0.2,
    "stability": 0.1
}

# Network simulation defaults
SIM_DEFAULTS = {
    "packet_loss": 0.0,
    "latency_ms": 0,
    "jitter_ms": 0,
    "enabled": False
}

# Server settings
API_HOST = os.getenv("API_HOST", "0.0.0.0")
API_PORT = int(os.getenv("API_PORT", 8080))
WS_PORT = int(os.getenv("WS_PORT", 8081))

# Link scan settings
SCAN_INTERVAL = 5.0  # seconds
SCAN_DURATION = 2.0  # seconds per link test
MIN_LINK_SCORE = 0.05  # Minimum score to consider a link usable (lowered for demo)

