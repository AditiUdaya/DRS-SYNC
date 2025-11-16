"""Network simulation layer for testing flaky connections"""
import asyncio
import random
import time
from typing import Optional, Dict
from dataclasses import dataclass, field
from datetime import datetime


@dataclass
class SimulatorConfig:
    """Configuration for network simulation"""
    packet_loss: float = 0.0  # 0.0 to 1.0
    latency_ms: float = 0.0
    jitter_ms: float = 0.0
    enabled: bool = False
    kill_link: bool = False  # Simulate complete link failure
    interface: Optional[str] = None  # Apply to specific interface, None = all


class NetworkSimulator:
    """Simulates network conditions (packet loss, latency, jitter)"""
    
    def __init__(self):
        self.configs: Dict[str, SimulatorConfig] = {}
        self.global_config = SimulatorConfig()
    
    def set_global_config(self, packet_loss: float = 0.0, latency_ms: float = 0.0, 
                         jitter_ms: float = 0.0, enabled: bool = False, kill_link: bool = False):
        """Set global simulation parameters"""
        self.global_config = SimulatorConfig(
            packet_loss=packet_loss,
            latency_ms=latency_ms,
            jitter_ms=jitter_ms,
            enabled=enabled,
            kill_link=kill_link
        )
    
    def set_interface_config(self, interface: str, packet_loss: float = 0.0, 
                            latency_ms: float = 0.0, jitter_ms: float = 0.0, 
                            enabled: bool = False, kill_link: bool = False):
        """Set simulation parameters for a specific interface"""
        self.configs[interface] = SimulatorConfig(
            packet_loss=packet_loss,
            latency_ms=latency_ms,
            jitter_ms=jitter_ms,
            enabled=enabled,
            kill_link=kill_link,
            interface=interface
        )
    
    def get_config(self, interface: Optional[str] = None) -> SimulatorConfig:
        """Get effective config for an interface (interface-specific or global)"""
        if interface and interface in self.configs:
            return self.configs[interface]
        return self.global_config
    
    async def simulate_send(self, interface: Optional[str] = None) -> bool:
        """
        Simulate sending a packet. Returns True if packet should be sent,
        False if it should be dropped (packet loss).
        """
        config = self.get_config(interface)
        
        if not config.enabled:
            return True
        
        if config.kill_link:
            return False
        
        # Simulate packet loss
        if random.random() < config.packet_loss:
            return False
        
        return True
    
    async def simulate_latency(self, interface: Optional[str] = None) -> float:
        """
        Simulate network latency. Returns delay in seconds.
        """
        config = self.get_config(interface)
        
        if not config.enabled:
            return 0.0
        
        if config.kill_link:
            return 10.0  # Long delay for killed link
        
        base_latency = config.latency_ms / 1000.0
        
        # Add jitter
        if config.jitter_ms > 0:
            jitter = (random.random() - 0.5) * 2 * (config.jitter_ms / 1000.0)
            total_latency = base_latency + jitter
        else:
            total_latency = base_latency
        
        return max(0.0, total_latency)
    
    async def wrap_send(self, send_func, data: bytes, interface: Optional[str] = None) -> bool:
        """
        Wrapper for send operations that applies simulation.
        Returns True if send was successful, False if dropped.
        """
        # Check if packet should be dropped
        if not await self.simulate_send(interface):
            return False
        
        # Apply latency
        delay = await self.simulate_latency(interface)
        if delay > 0:
            await asyncio.sleep(delay)
        
        # Perform actual send
        try:
            # send_func is a lambda that returns a coroutine from sock_sendto
            result = send_func(data)
            if asyncio.iscoroutine(result):
                await result
            return True
        except Exception as e:
            # Only print if there's an actual error message (not empty)
            error_msg = str(e).strip()
            if error_msg:
                print(f"Send error (simulated): {error_msg}")
            return False
    
    def kill_link(self, interface: Optional[str] = None):
        """Simulate killing a link"""
        if interface:
            if interface not in self.configs:
                self.configs[interface] = SimulatorConfig(interface=interface)
            self.configs[interface].kill_link = True
            self.configs[interface].enabled = True
        else:
            self.global_config.kill_link = True
            self.global_config.enabled = True
    
    def restore_link(self, interface: Optional[str] = None):
        """Restore a killed link"""
        if interface and interface in self.configs:
            self.configs[interface].kill_link = False
            # Also disable simulator if packet_loss is 0
            if self.configs[interface].packet_loss == 0.0:
                self.configs[interface].enabled = False
        else:
            self.global_config.kill_link = False
            # Also disable simulator if packet_loss is 0
            if self.global_config.packet_loss == 0.0:
                self.global_config.enabled = False
    
    def reset(self, interface: Optional[str] = None):
        """Reset simulation for an interface or globally"""
        if interface and interface in self.configs:
            del self.configs[interface]
        else:
            self.global_config = SimulatorConfig()

