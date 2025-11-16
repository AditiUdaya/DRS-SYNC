"""Reliable chunked transfer engine with adaptive uplink selection"""
import asyncio
import socket
import time
import zlib
from typing import Dict, List, Optional, Callable
from datetime import datetime
import aiofiles

from backend.config import (
    CHUNK_SIZE, SLIDING_WINDOW_SIZE, MAX_RETRIES, 
    RETRY_DELAY_BASE, ADAPTIVE_RTT_ALPHA
)
from backend.chunk_manager import ChunkManager, ChunkStatus, FileManifest
from backend.interface_scanner import InterfaceScanner, LinkMetrics
from backend.network_simulator import NetworkSimulator


class TransferEngine:
    """Main transfer engine with adaptive link selection and sliding window"""
    
    def __init__(self, chunk_manager: ChunkManager, interface_scanner: InterfaceScanner,
                 network_simulator: NetworkSimulator):
        self.chunk_manager = chunk_manager
        self.interface_scanner = interface_scanner
        self.network_simulator = network_simulator
        
        self.active_transfers: Dict[str, asyncio.Task] = {}
        self.transfer_stats: Dict[str, Dict] = {}
        self.paused_transfers: set = set()
        
        # Callback for progress updates
        self.progress_callback: Optional[Callable] = None
    
    def set_progress_callback(self, callback: Callable):
        """Set callback for progress updates"""
        self.progress_callback = callback
    
    async def start_transfer(self, file_id: str, destination_host: str, 
                           destination_port: int) -> bool:
        """Start transferring a file"""
        if file_id in self.active_transfers:
            return False
        
        manifest = self.chunk_manager.load_manifest(file_id)
        if not manifest:
            return False
        
        # Create transfer task
        task = asyncio.create_task(
            self._transfer_file(file_id, destination_host, destination_port)
        )
        self.active_transfers[file_id] = task
        
        # Initialize stats
        self.transfer_stats[file_id] = {
            "start_time": datetime.now(),
            "bytes_sent": 0,
            "bytes_original": 0,  # Original size before compression
            "chunks_sent": 0,
            "chunks_acked": 0,
            "retransmissions": 0,
            "link_switches": 0,
            "current_link": None,
            "compression_ratio": 0.0
        }
        
        return True
    
    async def pause_transfer(self, file_id: str):
        """Pause a transfer"""
        self.paused_transfers.add(file_id)
    
    async def resume_transfer(self, file_id: str):
        """Resume a paused transfer"""
        self.paused_transfers.discard(file_id)
    
    async def cancel_transfer(self, file_id: str):
        """Cancel a transfer"""
        if file_id in self.active_transfers:
            self.active_transfers[file_id].cancel()
            del self.active_transfers[file_id]
        self.paused_transfers.discard(file_id)
    
    async def _transfer_file(self, file_id: str, destination_host: str, 
                            destination_port: int):
        """Main transfer loop for a file"""
        manifest = self.chunk_manager.load_manifest(file_id)
        if not manifest:
            return
        
        # Wait for at least one link to be available (with timeout)
        max_wait = 10  # seconds
        waited = 0
        best_link = None
        while waited < max_wait:
            best_link = self.interface_scanner.get_best_link()
            if best_link:
                print(f"✅ Using best link: {best_link.interface} ({best_link.ip_address}) - Score: {best_link.link_score:.2%}")
                break
            # Force a scan if no links found
            print(f"⏳ Waiting for network links... ({waited}/{max_wait}s)")
            links = await self.interface_scanner.scan_all_interfaces()
            print(f"   Found {len(links)} interfaces: {[l.interface for l in links]}")
            await asyncio.sleep(1.0)
            waited += 1
        
        if not best_link:
            print(f"❌ Error: No network links available after {max_wait}s. Cannot start transfer for {file_id}.")
            return
        
        # Create socket (will bind to specific interface later)
        sock = None
        current_link: Optional[LinkMetrics] = None
        
        try:
            while not self.chunk_manager.is_complete(file_id):
                # Check if paused
                while file_id in self.paused_transfers:
                    await asyncio.sleep(0.5)
                
                # Get best available link
                best_link = self.interface_scanner.get_best_link()
                if not best_link:
                    # Try to rescan and wait a bit
                    await self.interface_scanner.scan_all_interfaces()
                    await asyncio.sleep(1.0)
                    best_link = self.interface_scanner.get_best_link()
                    if not best_link:
                        print(f"No available links for {file_id}, retrying...")
                        await asyncio.sleep(1.0)
                        continue
                
                # Switch link if needed (compare by interface name, not object reference)
                if current_link is None or current_link.interface != best_link.interface:
                    if sock:
                        sock.close()
                    sock = await self._create_socket_for_link(best_link)
                    current_link = best_link
                    self.transfer_stats[file_id]["current_link"] = best_link.interface
                    self.transfer_stats[file_id]["link_switches"] += 1
                    print(f"🔄 Switched to link: {best_link.interface} ({best_link.ip_address})")
                
                # Get chunks to send (sliding window)
                in_flight = self.chunk_manager.get_in_flight_chunks(file_id)
                available_slots = SLIDING_WINDOW_SIZE - len(in_flight)
                
                if available_slots > 0:
                    pending = self.chunk_manager.get_pending_chunks(file_id, limit=available_slots)
                    
                    if pending:
                        # Send chunks
                        send_tasks = [
                            self._send_chunk(file_id, chunk, sock, best_link, 
                                           destination_host, destination_port)
                            for chunk in pending
                        ]
                        
                        if send_tasks:
                            await asyncio.gather(*send_tasks, return_exceptions=True)
                            print(f"📤 Sent {len(send_tasks)} chunks for {file_id} to {destination_host}:{destination_port}")
                    else:
                        # No pending chunks - might be waiting for ACKs
                        if len(in_flight) > 0:
                            print(f"⏳ Waiting for ACKs: {len(in_flight)} chunks in flight for {file_id}")
                
                # Check for ACKs on the send socket (non-blocking) - do this after sending
                await self._check_for_acks(file_id, sock)
                
                # Check for timeouts and retransmit
                await self._check_timeouts(file_id, sock, best_link, 
                                         destination_host, destination_port)
                
                # Update progress
                if self.progress_callback:
                    progress = self.chunk_manager.get_progress(file_id)
                    await self.progress_callback(file_id, progress, self.transfer_stats[file_id])
                
                await asyncio.sleep(0.1)  # Small delay to prevent busy loop
            
            # Transfer complete
            manifest.completed_at = datetime.now()
            self.chunk_manager._save_manifest(manifest)
            
            if self.progress_callback:
                progress = self.chunk_manager.get_progress(file_id)
                await self.progress_callback(file_id, progress, self.transfer_stats[file_id])
        
        except asyncio.CancelledError:
            print(f"Transfer {file_id} cancelled")
        except Exception as e:
            print(f"Error in transfer {file_id}: {e}")
        finally:
            if sock:
                sock.close()
            if file_id in self.active_transfers:
                del self.active_transfers[file_id]
    
    async def _create_socket_for_link(self, link: LinkMetrics) -> socket.socket:
        """Create a socket bound to a specific network interface"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            # Try SO_BINDTODEVICE first (works on Linux with root)
            try:
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE, 
                              link.interface.encode())
            except (OSError, AttributeError):
                # Fallback: bind to the interface's IP address (works on macOS without root)
                # This achieves similar routing behavior without requiring elevation
                sock.bind((link.ip_address, 0))  # Bind to interface IP, OS picks port
        except Exception as e:
            print(f"Warning: Could not bind to {link.interface} ({link.ip_address}): {e}")
        sock.settimeout(5.0)
        return sock
    
    async def _send_chunk(self, file_id: str, chunk, sock: socket.socket, 
                         link: LinkMetrics, dest_host: str, dest_port: int):
        """Send a single chunk"""
        manifest = self.chunk_manager.load_manifest(file_id)
        if not manifest:
            return
        
        # Mark chunk as in flight (only if not already in flight to preserve sent_at)
        if manifest and chunk.chunk_id in manifest.chunks:
            current_chunk = manifest.chunks[chunk.chunk_id]
            if current_chunk.status != ChunkStatus.IN_FLIGHT:
                self.chunk_manager.update_chunk_status(
                    file_id, chunk.chunk_id, ChunkStatus.IN_FLIGHT, link.interface
                )
            # Update sent_at for this send attempt (for timeout tracking)
            current_chunk.sent_at = datetime.now()
        
        # Read chunk data
        try:
            async with aiofiles.open(manifest.file_path, 'rb') as f:
                await f.seek(chunk.offset)
                data = await f.read(chunk.size)
        except Exception as e:
            print(f"Error reading chunk {chunk.chunk_id}: {e}")
            self.chunk_manager.update_chunk_status(
                file_id, chunk.chunk_id, ChunkStatus.FAILED
            )
            return
        
        # Compress data if it helps (hash is always on original data)
        compressed_data = zlib.compress(data, level=6)  # Level 6 is a good balance
        is_compressed = len(compressed_data) < len(data)
        
        # Use compressed data if it's smaller, otherwise use original
        # Note: Hash is calculated on original data, not compressed
        if is_compressed:
            data_to_send = compressed_data
        else:
            data_to_send = data
        
        # Create packet: [file_id|chunk_id|offset|size|compressed_flag|compressed_size|hash|data]
        packet = self._create_chunk_packet(file_id, chunk, data_to_send, is_compressed, len(data))
        
        # Send with simulation
        try:
            success = await self.network_simulator.wrap_send(
                lambda d: asyncio.get_event_loop().sock_sendto(sock, d, (dest_host, dest_port)),
                packet,
                link.interface
            )
            
            if success:
                self.transfer_stats[file_id]["bytes_sent"] += len(packet)
                self.transfer_stats[file_id]["bytes_original"] += len(data)
                self.transfer_stats[file_id]["chunks_sent"] += 1
                # Update compression ratio
                if self.transfer_stats[file_id]["bytes_original"] > 0:
                    self.transfer_stats[file_id]["compression_ratio"] = (
                        self.transfer_stats[file_id]["bytes_sent"] / 
                        self.transfer_stats[file_id]["bytes_original"]
                    )
                # Send successful - sent_at already set above
                pass
            else:
                # Packet dropped by simulator - keep it IN_FLIGHT for retransmission
                # Don't increment retry_count here - let timeout logic handle retries
                # The timeout check will retransmit and track retries properly
                print(f"⚠️  Packet dropped by simulator for chunk {chunk.chunk_id} (will retry on timeout)")
                # Keep chunk in IN_FLIGHT state so it can be retransmitted
                # The timeout check will handle retransmission and retry counting
        except Exception as e:
            print(f"❌ Error sending chunk {chunk.chunk_id}: {e}")
            self.chunk_manager.update_chunk_status(
                file_id, chunk.chunk_id, ChunkStatus.FAILED
            )
    
    def _create_chunk_packet(self, file_id: str, chunk, data: bytes, is_compressed: bool = False, original_size: int = 0) -> bytes:
        """Create a packet for a chunk"""
        # Packet format: [file_id_len|file_id|chunk_id|offset|original_size|compressed_flag|compressed_size|hash|data]
        file_id_bytes = file_id.encode('utf-8')
        hash_bytes = bytes.fromhex(chunk.hash)
        
        packet = (
            len(file_id_bytes).to_bytes(1, 'big') +
            file_id_bytes +
            chunk.chunk_id.to_bytes(4, 'big') +
            chunk.offset.to_bytes(8, 'big') +
            chunk.size.to_bytes(4, 'big') +  # Original size
            (1 if is_compressed else 0).to_bytes(1, 'big') +  # Compression flag
            len(data).to_bytes(4, 'big') +  # Compressed/actual size
            hash_bytes +
            data
        )
        return packet
    
    async def _check_timeouts(self, file_id: str, sock: socket.socket, 
                             link: LinkMetrics, dest_host: str, dest_port: int):
        """Check for timed-out chunks and retransmit"""
        in_flight = self.chunk_manager.get_in_flight_chunks(file_id)
        now = datetime.now()
        
        for chunk in in_flight:
            if chunk.sent_at:
                elapsed = (now - chunk.sent_at).total_seconds()
                timeout = RETRY_DELAY_BASE * (2 ** chunk.retry_count)  # Exponential backoff
                
                if elapsed > timeout:
                    # Timeout - retransmit if retries available
                    if chunk.retry_count < MAX_RETRIES:
                        chunk.retry_count += 1
                        print(f"⏱️  Timeout for chunk {chunk.chunk_id}, retrying ({chunk.retry_count}/{MAX_RETRIES})")
                        await self._send_chunk(file_id, chunk, sock, link, dest_host, dest_port)
                        self.transfer_stats[file_id]["retransmissions"] += 1
                    else:
                        # Max retries exceeded - mark as failed
                        print(f"❌ Max retries exceeded for chunk {chunk.chunk_id}, marking as FAILED")
                        self.chunk_manager.update_chunk_status(
                            file_id, chunk.chunk_id, ChunkStatus.FAILED
                        )
    
    def get_transfer_status(self, file_id: str) -> Optional[Dict]:
        """Get status of a transfer"""
        if file_id not in self.transfer_stats:
            return None
        
        progress = self.chunk_manager.get_progress(file_id)
        stats = self.transfer_stats[file_id].copy()
        stats.update(progress)
        stats["is_active"] = file_id in self.active_transfers
        stats["is_paused"] = file_id in self.paused_transfers
        
        if stats.get("start_time"):
            elapsed = (datetime.now() - stats["start_time"]).total_seconds()
            if elapsed > 0 and stats.get("bytes_transferred", 0) > 0:
                stats["throughput_mbps"] = (stats["bytes_transferred"] * 8) / (elapsed * 1_000_000)
            else:
                stats["throughput_mbps"] = 0.0
        
        return stats
    
    async def _check_for_acks(self, file_id: str, sock: socket.socket):
        """Check for ACK packets on the socket (non-blocking)"""
        if not sock:
            return
        
        try:
            # Check multiple times for ACKs (they might arrive quickly)
            loop = asyncio.get_event_loop()
            old_timeout = sock.gettimeout()
            sock.settimeout(0.0)  # Non-blocking
            
            # Try to receive multiple ACKs (up to 10 at a time)
            for _ in range(10):
                try:
                    data, addr = await loop.sock_recvfrom(sock, 1024)
                    
                    # Parse ACK: [ACK_MAGIC|file_id_len|file_id|chunk_id]
                    if len(data) >= 4 and data[:3] == b'ACK':
                        pos = 3
                        file_id_len = data[pos]
                        pos += 1
                        
                        if pos + file_id_len > len(data):
                            continue
                        
                        ack_file_id = data[pos:pos+file_id_len].decode('utf-8')
                        pos += file_id_len
                        
                        if pos + 4 > len(data):
                            continue
                        
                        if ack_file_id == file_id:
                            chunk_id = int.from_bytes(data[pos:pos+4], 'big')
                            
                            # Mark chunk as ACKED
                            self.chunk_manager.update_chunk_status(
                                file_id, chunk_id, ChunkStatus.ACKED
                            )
                            self.transfer_stats[file_id]["chunks_acked"] += 1
                            
                            print(f"✅ ACK received for chunk {chunk_id} of {file_id}")
                except (socket.timeout, BlockingIOError, OSError):
                    # No more data available
                    break
                except Exception as e:
                    # Skip malformed packets
                    continue
        finally:
            # Restore timeout
            try:
                sock.settimeout(old_timeout if old_timeout else 5.0)
            except:
                pass

