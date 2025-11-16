"""REST API and WebSocket server for DRS-SYNC"""
import asyncio
import json
import uuid
from pathlib import Path
from typing import Dict, List, Optional
from datetime import datetime

from fastapi import FastAPI, UploadFile, File, HTTPException, WebSocket, WebSocketDisconnect, Form
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from pydantic import BaseModel
import aiofiles

from backend.config import API_HOST, API_PORT, WS_PORT, UPLOAD_DIR
from backend.interface_scanner import InterfaceScanner
from backend.chunk_manager import ChunkManager
from backend.network_simulator import NetworkSimulator
from backend.transfer_engine import TransferEngine


# Pydantic models
class TransferRequest(BaseModel):
    file_id: str
    destination_host: str
    destination_port: int


class SimulatorConfig(BaseModel):
    packet_loss: float = 0.0
    latency_ms: float = 0.0
    jitter_ms: float = 0.0
    enabled: bool = False
    kill_link: bool = False
    interface: Optional[str] = None


class PriorityUpdate(BaseModel):
    priority: str  # high, standard, background


# Initialize components
app = FastAPI(title="DRS-SYNC API", version="1.0.0")

# CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Global instances
interface_scanner = InterfaceScanner()
chunk_manager = ChunkManager()
network_simulator = NetworkSimulator()
transfer_engine = TransferEngine(chunk_manager, interface_scanner, network_simulator)

# WebSocket connections
websocket_connections: List[WebSocket] = []


def serialize_datetime(obj):
    """Recursively serialize datetime objects in a dict/list"""
    if isinstance(obj, datetime):
        return obj.isoformat()
    elif isinstance(obj, dict):
        return {k: serialize_datetime(v) for k, v in obj.items()}
    elif isinstance(obj, list):
        return [serialize_datetime(item) for item in obj]
    else:
        return obj


async def broadcast_telemetry(data: dict):
    """Broadcast telemetry data to all connected WebSocket clients"""
    if not websocket_connections:
        return
    
    # Serialize any datetime objects before JSON encoding
    serializable_data = serialize_datetime(data)
    message = json.dumps(serializable_data)
    disconnected = []
    
    for ws in websocket_connections:
        try:
            await ws.send_text(message)
        except Exception as e:
            print(f"Error sending to WebSocket: {e}")
            disconnected.append(ws)
    
    # Remove disconnected clients
    for ws in disconnected:
        websocket_connections.remove(ws)


async def progress_callback(file_id: str, progress: dict, stats: dict):
    """Callback for transfer progress updates"""
    await broadcast_telemetry({
        "type": "transfer_progress",
        "file_id": file_id,
        "progress": progress,
        "stats": stats
    })


transfer_engine.set_progress_callback(progress_callback)


@app.on_event("startup")
async def startup():
    """Startup tasks"""
    # Start continuous interface scanning
    await interface_scanner.start_continuous_scan()
    
    # Start telemetry broadcast loop
    asyncio.create_task(telemetry_broadcast_loop())


@app.on_event("shutdown")
async def shutdown():
    """Shutdown tasks"""
    await interface_scanner.stop_continuous_scan()


async def telemetry_broadcast_loop():
    """Periodic telemetry broadcast"""
    while True:
        try:
            # Get link metrics
            links = await interface_scanner.scan_all_interfaces()
            link_data = [
                {
                    "interface": m.interface,
                    "ip_address": m.ip_address,
                    "throughput_mbps": m.throughput_mbps,
                    "rtt_ms": m.rtt_ms,
                    "packet_loss": m.packet_loss,
                    "jitter_ms": m.jitter_ms,
                    "stability_score": m.stability_score,
                    "link_score": m.link_score,
                    "is_active": m.is_active
                }
                for m in links
            ]
            
            await broadcast_telemetry({
                "type": "link_metrics",
                "links": link_data,
                "timestamp": datetime.now().isoformat()
            })
            
            await asyncio.sleep(2.0)  # Update every 2 seconds
        except Exception as e:
            print(f"Error in telemetry loop: {e}")
            await asyncio.sleep(2.0)


# REST API Endpoints

@app.get("/")
async def root():
    """Root endpoint"""
    return {
        "name": "DRS-SYNC",
        "version": "1.0.0",
        "status": "running"
    }


@app.get("/api/links")
async def get_links():
    """Get all network links and their metrics"""
    links = await interface_scanner.scan_all_interfaces()
    return {
        "links": [
            {
                "interface": m.interface,
                "ip_address": m.ip_address,
                "throughput_mbps": m.throughput_mbps,
                "rtt_ms": m.rtt_ms,
                "packet_loss": m.packet_loss,
                "jitter_ms": m.jitter_ms,
                "stability_score": m.stability_score,
                "link_score": m.link_score,
                "is_active": m.is_active
            }
            for m in links
        ]
    }


@app.get("/api/links/best")
async def get_best_link():
    """Get the best performing link"""
    best = interface_scanner.get_best_link()
    if not best:
        raise HTTPException(status_code=404, detail="No active links available")
    
    return {
        "interface": best.interface,
        "ip_address": best.ip_address,
        "throughput_mbps": best.throughput_mbps,
        "rtt_ms": best.rtt_ms,
        "packet_loss": best.packet_loss,
        "link_score": best.link_score
    }


@app.post("/api/files/upload")
async def upload_file(file: UploadFile = File(...), priority: str = Form("standard")):
    """Upload a file to the transfer queue"""
    if priority not in ["high", "standard", "background"]:
        raise HTTPException(status_code=400, detail="Invalid priority")
    
    # Generate file ID
    file_id = str(uuid.uuid4())
    file_path = UPLOAD_DIR / f"{file_id}_{file.filename}"
    
    # Save file
    file_size = 0
    async with aiofiles.open(file_path, 'wb') as f:
        while True:
            chunk = await file.read(8192)
            if not chunk:
                break
            await f.write(chunk)
            file_size += len(chunk)
    
    # Create manifest
    manifest = chunk_manager.create_manifest(file_id, str(file_path), file_size, priority)
    
    return {
        "file_id": file_id,
        "filename": file.filename,
        "size": file_size,
        "priority": priority,
        "total_chunks": manifest.total_chunks,
        "status": "queued"
    }


@app.get("/api/files")
async def list_files():
    """List all files in the system"""
    files = []
    for file_id in chunk_manager.manifests.keys():
        manifest = chunk_manager.load_manifest(file_id)
        if manifest:
            progress = chunk_manager.get_progress(file_id)
            status = transfer_engine.get_transfer_status(file_id)
            
            files.append({
                "file_id": file_id,
                "filename": Path(manifest.file_path).name,
                "size": manifest.file_size,
                "priority": manifest.priority,
                "progress": progress,
                "status": "active" if status and status.get("is_active") else "queued",
                "is_paused": status.get("is_paused") if status else False
            })
    
    return {"files": files}


@app.get("/api/files/{file_id}")
async def get_file_status(file_id: str):
    """Get status of a specific file transfer"""
    manifest = chunk_manager.load_manifest(file_id)
    if not manifest:
        raise HTTPException(status_code=404, detail="File not found")
    
    progress = chunk_manager.get_progress(file_id)
    status = transfer_engine.get_transfer_status(file_id)
    
    # Serialize datetime objects in status
    if status:
        status = serialize_datetime(status)
    
    return {
        "file_id": file_id,
        "filename": Path(manifest.file_path).name,
        "size": manifest.file_size,
        "priority": manifest.priority,
        "progress": progress,
        "status": status
    }


@app.post("/api/files/{file_id}/transfer")
async def start_transfer(file_id: str, request: TransferRequest):
    """Start transferring a file"""
    if file_id != request.file_id:
        raise HTTPException(status_code=400, detail="File ID mismatch")
    
    success = await transfer_engine.start_transfer(
        file_id, request.destination_host, request.destination_port
    )
    
    if not success:
        raise HTTPException(status_code=400, detail="Failed to start transfer")
    
    return {"status": "started", "file_id": file_id}


@app.post("/api/files/{file_id}/pause")
async def pause_transfer(file_id: str):
    """Pause a transfer"""
    await transfer_engine.pause_transfer(file_id)
    return {"status": "paused", "file_id": file_id}


@app.post("/api/files/{file_id}/resume")
async def resume_transfer(file_id: str):
    """Resume a transfer"""
    await transfer_engine.resume_transfer(file_id)
    return {"status": "resumed", "file_id": file_id}


@app.post("/api/files/{file_id}/cancel")
async def cancel_transfer(file_id: str):
    """Cancel a transfer"""
    await transfer_engine.cancel_transfer(file_id)
    return {"status": "cancelled", "file_id": file_id}


@app.put("/api/files/{file_id}/priority")
async def update_priority(file_id: str, update: PriorityUpdate):
    """Update priority of a file"""
    manifest = chunk_manager.load_manifest(file_id)
    if not manifest:
        raise HTTPException(status_code=404, detail="File not found")
    
    if update.priority not in ["high", "standard", "background"]:
        raise HTTPException(status_code=400, detail="Invalid priority")
    
    manifest.priority = update.priority
    chunk_manager._save_manifest(manifest)
    
    return {"status": "updated", "file_id": file_id, "priority": update.priority}


# Simulator endpoints

@app.post("/api/simulator/config")
async def set_simulator_config(config: SimulatorConfig):
    """Configure network simulator"""
    if config.interface:
        network_simulator.set_interface_config(
            config.interface, config.packet_loss, config.latency_ms,
            config.jitter_ms, config.enabled, config.kill_link
        )
    else:
        network_simulator.set_global_config(
            config.packet_loss, config.latency_ms, config.jitter_ms,
            config.enabled, config.kill_link
        )
    
    # Log simulator status
    if config.interface:
        print(f"📡 Simulator config for {config.interface}: enabled={config.enabled}, loss={config.packet_loss*100:.1f}%")
    else:
        print(f"📡 Global simulator config: enabled={config.enabled}, loss={config.packet_loss*100:.1f}%")
    
    return {"status": "configured", "config": config.dict()}


@app.post("/api/simulator/kill-link")
async def kill_link(interface: Optional[str] = None):
    """Kill a link (simulate failure)"""
    network_simulator.kill_link(interface)
    return {"status": "link_killed", "interface": interface}


@app.post("/api/simulator/restore-link")
async def restore_link(interface: Optional[str] = None):
    """Restore a killed link"""
    network_simulator.restore_link(interface)
    print(f"✅ Link restored: {interface or 'all'}")
    return {"status": "link_restored", "interface": interface}


@app.post("/api/simulator/reset")
async def reset_simulator(interface: Optional[str] = None):
    """Reset simulator"""
    network_simulator.reset(interface)
    return {"status": "reset", "interface": interface}


# WebSocket endpoint

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """WebSocket endpoint for real-time telemetry"""
    await websocket.accept()
    websocket_connections.append(websocket)
    
    try:
        while True:
            # Keep connection alive and handle any incoming messages
            data = await websocket.receive_text()
            # Echo back or handle commands
            await websocket.send_text(json.dumps({"type": "pong", "data": data}))
    except WebSocketDisconnect:
        websocket_connections.remove(websocket)


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host=API_HOST, port=API_PORT)

