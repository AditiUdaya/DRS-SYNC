#!/usr/bin/env python3
"""Find your PC's IP address and network interfaces for DRS-SYNC transfers"""
import sys
import socket
import psutil

def get_network_interfaces():
    """Get all active network interfaces with IP addresses"""
    interfaces = []
    
    # Get all network interfaces
    addrs = psutil.net_if_addrs()
    stats = psutil.net_if_stats()
    
    for iface_name, iface_addrs in addrs.items():
        # Skip loopback
        if iface_name == 'lo' or iface_name.startswith('lo'):
            continue
        
        iface_stat = stats.get(iface_name)
        if not iface_stat or not iface_stat.isup:
            continue
        
        # Get IPv4 address
        ip_address = None
        for addr in iface_addrs:
            if addr.family == socket.AF_INET:
                ip_address = addr.address
                break
        
        if ip_address:
            # Determine interface type
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
                'isup': iface_stat.isup,
                'speed': iface_stat.speed if iface_stat.speed > 0 else None
            })
    
    return interfaces

def get_best_interface(interfaces):
    """Determine the best interface for transfers"""
    if not interfaces:
        return None
    
    # Prefer Ethernet over WiFi, WiFi over others
    priority = {
        'Ethernet/WiFi': 3,
        'WiFi': 2,
        'USB Adapter': 2,
        'Cellular/Modem': 1,
        'VPN Tunnel': 0,
        'Unknown': 1
    }
    
    # Sort by priority, then by speed
    sorted_interfaces = sorted(
        interfaces,
        key=lambda x: (priority.get(x['type'], 0), x['speed'] or 0),
        reverse=True
    )
    
    return sorted_interfaces[0] if sorted_interfaces else None

def main():
    print("=" * 70)
    print("DRS-SYNC Network Interface Discovery")
    print("=" * 70)
    print()
    
    interfaces = get_network_interfaces()
    
    if not interfaces:
        print("❌ No active network interfaces found!")
        print("   Make sure your network adapter is connected and enabled.")
        sys.exit(1)
    
    print(f"✅ Found {len(interfaces)} active network interface(s):\n")
    
    for i, iface in enumerate(interfaces, 1):
        speed_str = f" ({iface['speed']} Mbps)" if iface['speed'] else ""
        print(f"  {i}. {iface['name']:15} {iface['ip']:20} [{iface['type']}]{speed_str}")
    
    print()
    best = get_best_interface(interfaces)
    
    if best:
        print("=" * 70)
        print("📡 RECOMMENDED FOR TRANSFERS:")
        print("=" * 70)
        print(f"   Interface: {best['name']}")
        print(f"   IP Address: {best['ip']}")
        print(f"   Type: {best['type']}")
        if best['speed']:
            print(f"   Speed: {best['speed']} Mbps")
        print()
        print("💡 Use this IP address when starting transfers from another PC")
        print()
        print("=" * 70)
        print("Example usage:")
        print("=" * 70)
        print(f"  Receiver PC: python3 -m backend.receiver 9000")
        print(f"  Sender PC:   Start transfer to {best['ip']}:9000")
        print()
    else:
        print("⚠️  Could not determine best interface")
    
    # Also show how to get hostname
    try:
        hostname = socket.gethostname()
        print(f"💻 Hostname: {hostname}")
        print(f"   You can also use '{hostname}' if DNS is configured")
    except:
        pass

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

