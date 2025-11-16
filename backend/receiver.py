"""Simple UDP receiver for testing DRS-SYNC transfers"""
import socket
import struct
import hashlib
import zlib
import xxhash
import psutil
from pathlib import Path
from typing import Dict
import json

from backend.config import UPLOAD_DIR


class ChunkReceiver:
    """Receives and reconstructs files from chunks"""
    
    def __init__(self, port: int = 9000, output_dir: Path = None):
        self.port = port
        self.output_dir = output_dir or (Path(__file__).parent.parent / "received")
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.received_chunks: Dict[str, Dict[int, bytes]] = {}
        self.file_manifests: Dict[str, dict] = {}
        
    def start(self):
        """Start receiving chunks"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        # Try to bind to the port, with helpful error message if it fails
        try:
            sock.bind(('0.0.0.0', self.port))
        except OSError as e:
            if e.errno == 48:  # Address already in use
                print(f"❌ Error: Port {self.port} is already in use.")
                print(f"   Another receiver may be running, or another process is using this port.")
                print(f"\n   Solutions:")
                print(f"   1. Use a different port:")
                print(f"      python3 -m backend.receiver 9001")
                print(f"   2. Find and stop the process using port {self.port}:")
                print(f"      lsof -ti:{self.port} | xargs kill")
                print(f"   3. Or check what's using the port:")
                print(f"      lsof -i:{self.port}")
                sock.close()
                return
            else:
                raise
        
        sock.settimeout(1.0)
        
        # Get and display network interface information
        interfaces = self._get_active_interfaces()
        best_interface = self._get_best_interface(interfaces)
        
        print("=" * 70)
        print("📡 DRS-SYNC Receiver Started")
        print("=" * 70)
        print(f"✅ Listening on port: {self.port}")
        print(f"📁 Output directory: {self.output_dir}")
        print()
        
        if best_interface:
            print("🌐 Network Information:")
            print(f"   Primary IP: {best_interface['ip']}")
            print(f"   Interface: {best_interface['name']} ({best_interface['type']})")
            print()
            print("💡 Share this IP address with the sender PC:")
            print(f"   {best_interface['ip']}:{self.port}")
            print()
        
        if len(interfaces) > 1:
            print("📋 All available interfaces:")
            for iface in interfaces:
                marker = " ⭐" if iface == best_interface else ""
                print(f"   {iface['name']:15} {iface['ip']:20} [{iface['type']}]{marker}")
            print()
        
        print("=" * 70)
        print("Waiting for incoming chunks...")
        print("Press Ctrl+C to stop")
        print("=" * 70)
        print()
        
        while True:
            try:
                data, addr = sock.recvfrom(65536)
                self._process_chunk(data, addr, sock)
            except socket.timeout:
                continue
            except KeyboardInterrupt:
                print("\nShutting down receiver...")
                break
            except Exception as e:
                print(f"Error receiving chunk: {e}")
        
        sock.close()
    
    def _process_chunk(self, data: bytes, addr: tuple, sock: socket.socket):
        """Process a received chunk"""
        try:
            # Parse packet: [file_id_len|file_id|chunk_id|offset|original_size|compressed_flag|compressed_size|hash|data]
            pos = 0
            
            file_id_len = data[pos]
            pos += 1
            
            file_id = data[pos:pos+file_id_len].decode('utf-8')
            pos += file_id_len
            
            chunk_id = struct.unpack('>I', data[pos:pos+4])[0]
            pos += 4
            
            offset = struct.unpack('>Q', data[pos:pos+8])[0]
            pos += 8
            
            original_size = struct.unpack('>I', data[pos:pos+4])[0]
            pos += 4
            
            is_compressed = struct.unpack('>B', data[pos:pos+1])[0] == 1
            pos += 1
            
            compressed_size = struct.unpack('>I', data[pos:pos+4])[0]
            pos += 4
            
            chunk_hash = data[pos:pos+8].hex()
            pos += 8
            
            chunk_data = data[pos:pos+compressed_size]
            
            # Decompress if needed
            if is_compressed:
                try:
                    chunk_data = zlib.decompress(chunk_data)
                    print(f"Decompressed chunk {chunk_id} from {compressed_size} to {len(chunk_data)} bytes")
                except Exception as e:
                    print(f"Error decompressing chunk {chunk_id}: {e}")
                    return
            
            # Verify chunk hash (on decompressed data)
            computed_hash = xxhash.xxh64(chunk_data).hexdigest()
            if computed_hash != chunk_hash:
                print(f"Hash mismatch for chunk {chunk_id} of {file_id}")
                return
            
            # Store chunk
            if file_id not in self.received_chunks:
                self.received_chunks[file_id] = {}
            
            self.received_chunks[file_id][chunk_id] = chunk_data
            
            print(f"Received chunk {chunk_id} from {file_id} ({len(chunk_data)} bytes) from {addr[0]}:{addr[1]}")
            
            # Send ACK back to sender
            self._send_ack(sock, file_id, chunk_id, addr)
            
            # Check if we have all chunks (simple heuristic - in production, use manifest)
            # For now, just save when we receive a chunk with offset 0 and size matches expected
            
        except Exception as e:
            print(f"Error processing chunk: {e}")
    
    def _get_active_interfaces(self):
        """Get all active network interfaces with IP addresses"""
        interfaces = []
        addrs = psutil.net_if_addrs()
        stats = psutil.net_if_stats()
        
        for iface_name, iface_addrs in addrs.items():
            if iface_name == 'lo' or iface_name.startswith('lo'):
                continue
            
            iface_stat = stats.get(iface_name)
            if not iface_stat or not iface_stat.isup:
                continue
            
            ip_address = None
            for addr in iface_addrs:
                if addr.family == socket.AF_INET:
                    ip_address = addr.address
                    break
            
            if ip_address:
                iface_type = "Unknown"
                if iface_name.startswith('en') or iface_name.startswith('eth'):
                    iface_type = "Ethernet/WiFi"
                elif iface_name.startswith('wlan') or iface_name.startswith('wifi'):
                    iface_type = "WiFi"
                elif iface_name.startswith('ppp') or iface_name.startswith('wwan'):
                    iface_type = "Cellular/Modem"
                elif iface_name.startswith('tun') or iface_name.startswith('tap'):
                    iface_type = "VPN Tunnel"
                elif iface_name.startswith('usb'):
                    iface_type = "USB Adapter"
                
                interfaces.append({
                    'name': iface_name,
                    'ip': ip_address,
                    'type': iface_type,
                    'speed': iface_stat.speed if iface_stat.speed > 0 else None
                })
        
        return interfaces
    
    def _get_best_interface(self, interfaces):
        """Determine the best interface for transfers"""
        if not interfaces:
            return None
        
        priority = {
            'Ethernet/WiFi': 3,
            'WiFi': 2,
            'USB Adapter': 2,
            'Cellular/Modem': 1,
            'VPN Tunnel': 0,
            'Unknown': 1
        }
        
        sorted_interfaces = sorted(
            interfaces,
            key=lambda x: (priority.get(x['type'], 0), x['speed'] or 0),
            reverse=True
        )
        
        return sorted_interfaces[0] if sorted_interfaces else None
    
    def _send_ack(self, sock: socket.socket, file_id: str, chunk_id: int, addr: tuple):
        """Send ACK packet back to sender"""
        try:
            # ACK format: [ACK_MAGIC|file_id_len|file_id|chunk_id]
            ack_magic = b'ACK'
            file_id_bytes = file_id.encode('utf-8')
            ack_packet = (
                ack_magic +
                len(file_id_bytes).to_bytes(1, 'big') +
                file_id_bytes +
                chunk_id.to_bytes(4, 'big')
            )
            sock.sendto(ack_packet, addr)
        except Exception as e:
            print(f"Error sending ACK: {e}")
    
    def save_file(self, file_id: str):
        """Save a complete file from received chunks"""
        if file_id not in self.received_chunks:
            print(f"No chunks received for {file_id}")
            return
        
        chunks = self.received_chunks[file_id]
        if not chunks:
            print(f"No chunks to save for {file_id}")
            return
        
        # Sort chunks by chunk_id
        sorted_chunk_ids = sorted(chunks.keys())
        
        # Reconstruct file
        output_path = self.output_dir / f"{file_id}_received.bin"
        with open(output_path, 'wb') as f:
            for chunk_id in sorted_chunk_ids:
                f.write(chunks[chunk_id])
        
        print(f"Saved file {file_id} to {output_path}")


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) > 1:
        try:
            port = int(sys.argv[1])
        except ValueError:
            print(f"❌ Error: Invalid port number '{sys.argv[1]}'")
            print("Usage: python3 -m backend.receiver [port]")
            sys.exit(1)
    else:
        port = 9000
    
    receiver = ChunkReceiver(port=port)
    receiver.start()

