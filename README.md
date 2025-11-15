# DRS-SYNC
---
## meta-parsing — Implement META packet sender and receiver for reliable UDP

### Summary
Added the first functional part of the reliable UDP transfer protocol: **META packet exchange**.  
Sender now sends file metadata, and receiver correctly parses it and prepares for chunk reception.

### Details
**Sender:**
- Reads filename and filesize
- Builds META payload (`filename + "\n" + filesize`)
- Packs header using `PacketHeader` (type = 10)
- Sends META packet via UDP to receiver port 5000

**Receiver:**
- Listens on UDP port 5000
- Receives and unpacks `PacketHeader`
- Detects META (`type = 10`)
- Parses filename, filesize, and computes total chunks
- Logs metadata for debugging

**Protocol Infrastructure:**
- Added `reliable_packet.hpp` with:
  - `PacketHeader` struct
  - `pack_header()` and `unpack_header()` helpers
  - Windows-safe endianness conversions (using winsock2)
- Removed all Linux-only includes (`<arpa/inet.h>`)
- Added `<winsock2.h>` and `<ws2tcpip.h>` to support MSYS2 UCRT64

---

