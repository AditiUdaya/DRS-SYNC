#!/usr/bin/env python3
"""Demo script for DRS-SYNC"""
import asyncio
import sys
import time
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from backend.chunk_manager import ChunkManager
from backend.interface_scanner import InterfaceScanner
from backend.network_simulator import NetworkSimulator
from backend.transfer_engine import TransferEngine
from backend.config import UPLOAD_DIR


async def demo_baseline_burst():
    """Demo 1: Baseline burst transfer"""
    print("\n" + "="*60)
    print("DEMO 1: Baseline Burst Transfer")
    print("="*60)
    
    # Create a test file
    test_file = UPLOAD_DIR / "test_500mb.bin"
    if not test_file.exists():
        print(f"Creating test file: {test_file}")
        with open(test_file, 'wb') as f:
            # Create 10MB file for demo (instead of 500MB)
            chunk = b'X' * (1024 * 1024)  # 1MB chunk
            for _ in range(10):
                f.write(chunk)
        print(f"Created {test_file.stat().st_size / 1024 / 1024:.2f} MB test file")
    
    chunk_manager = ChunkManager()
    interface_scanner = InterfaceScanner()
    network_simulator = NetworkSimulator()
    transfer_engine = TransferEngine(chunk_manager, interface_scanner, network_simulator)
    
    # Scan interfaces
    print("\nScanning network interfaces...")
    links = await interface_scanner.scan_all_interfaces()
    for link in links:
        print(f"  {link.interface}: {link.link_score:.2%} score, {link.throughput_mbps:.2f} Mbps")
    
    # Create manifest
    file_id = "demo1_test_file"
    manifest = chunk_manager.create_manifest(
        str(test_file), str(test_file), test_file.stat().st_size, "high"
    )
    print(f"\nCreated manifest: {manifest.total_chunks} chunks")
    
    print("\nStarting transfer...")
    print("(In real demo, this would transfer to receiver)")
    print("Demo complete!\n")


async def demo_kill_uplink():
    """Demo 2: Kill uplink and watch adaptation"""
    print("\n" + "="*60)
    print("DEMO 2: Kill Uplink - Watch Auto-Adaptation")
    print("="*60)
    
    interface_scanner = InterfaceScanner()
    network_simulator = NetworkSimulator()
    
    # Scan interfaces
    links = await interface_scanner.scan_all_interfaces()
    if not links:
        print("No network interfaces found!")
        return
    
    best_link = links[0]
    print(f"\nBest link: {best_link.interface} (score: {best_link.link_score:.2%})")
    
    # Simulate packet loss
    print("\nSimulating 70% packet loss...")
    network_simulator.set_interface_config(
        best_link.interface, packet_loss=0.7, enabled=True
    )
    
    print("Transfer would slow down and adapt...")
    await asyncio.sleep(2)
    
    # Restore
    network_simulator.reset(best_link.interface)
    print("Link restored - transfer resumes at full speed")
    print("Demo complete!\n")


async def demo_resume():
    """Demo 3: Crash and resume"""
    print("\n" + "="*60)
    print("DEMO 3: Crash & Resume from Checkpoint")
    print("="*60)
    
    chunk_manager = ChunkManager()
    
    # Simulate a transfer that was interrupted
    test_file = UPLOAD_DIR / "test_resume.bin"
    if not test_file.exists():
        with open(test_file, 'wb') as f:
            chunk = b'Y' * (1024 * 1024)
            for _ in range(5):
                f.write(chunk)
    
    file_id = "demo3_resume_file"
    manifest = chunk_manager.create_manifest(
        file_id, str(test_file), test_file.stat().st_size, "standard"
    )
    
    # Simulate some chunks being ACKed
    print(f"\nSimulating partial transfer...")
    total_chunks = manifest.total_chunks
    acked_chunks = total_chunks // 2
    
    from backend.chunk_manager import ChunkStatus
    for i in range(acked_chunks):
        chunk_manager.update_chunk_status(file_id, i, ChunkStatus.ACKED)
    
    progress = chunk_manager.get_progress(file_id)
    print(f"Progress before crash: {progress['progress']:.1%}")
    print(f"Chunks complete: {progress['chunks_complete']} / {progress['chunks_total']}")
    
    print("\n[Simulating crash...]")
    await asyncio.sleep(1)
    
    print("\n[Restarting...]")
    # Reload manifest
    loaded_manifest = chunk_manager.load_manifest(file_id)
    if loaded_manifest:
        progress = chunk_manager.get_progress(file_id)
        print(f"Resumed from checkpoint: {progress['progress']:.1%}")
        print(f"Remaining chunks: {progress['chunks_total'] - progress['chunks_complete']}")
        print("Transfer continues from last checkpoint!")
    
    print("Demo complete!\n")


async def demo_multi_uplink():
    """Demo 4: Multi-uplink switching"""
    print("\n" + "="*60)
    print("DEMO 4: Multi-Uplink Switching")
    print("="*60)
    
    interface_scanner = InterfaceScanner()
    network_simulator = NetworkSimulator()
    
    # Scan all interfaces
    print("\nScanning all network interfaces...")
    links = await interface_scanner.scan_all_interfaces()
    
    if len(links) < 2:
        print("Need at least 2 interfaces for this demo")
        return
    
    print(f"\nFound {len(links)} interfaces:")
    for i, link in enumerate(links):
        print(f"  {i+1}. {link.interface}: {link.link_score:.2%} score")
    
    primary = links[0]
    secondary = links[1] if len(links) > 1 else links[0]
    
    print(f"\nPrimary link: {primary.interface}")
    print(f"Secondary link: {secondary.interface}")
    
    print("\n[Simulating WiFi disconnection...]")
    network_simulator.kill_link(primary.interface)
    await asyncio.sleep(1)
    
    # Rescan to see new best link
    links = await interface_scanner.scan_all_interfaces()
    new_best = interface_scanner.get_best_link()
    
    if new_best:
        print(f"System automatically switched to: {new_best.interface}")
        print("Transfer continues seamlessly!")
    
    network_simulator.restore_link(primary.interface)
    print("\nDemo complete!\n")


async def main():
    """Run all demos"""
    print("\n" + "="*60)
    print("🚦 DRS-SYNC DEMO SUITE")
    print("="*60)
    
    demos = [
        ("Baseline Burst", demo_baseline_burst),
        ("Kill Uplink", demo_kill_uplink),
        ("Crash & Resume", demo_resume),
        ("Multi-Uplink Switch", demo_multi_uplink),
    ]
    
    for name, demo_func in demos:
        try:
            await demo_func()
            await asyncio.sleep(1)
        except Exception as e:
            print(f"Error in {name} demo: {e}")
            import traceback
            traceback.print_exc()
    
    print("\n" + "="*60)
    print("All demos complete!")
    print("="*60 + "\n")


if __name__ == "__main__":
    asyncio.run(main())

