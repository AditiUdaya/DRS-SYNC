"""Chunk management and manifest system for resumable transfers"""
import json
import hashlib
import xxhash
from pathlib import Path
from typing import Dict, List, Optional, Set
from dataclasses import dataclass, asdict
from datetime import datetime
from enum import Enum

from backend.config import CHUNK_SIZE, MANIFEST_DIR


class ChunkStatus(Enum):
    PENDING = "pending"
    IN_FLIGHT = "in_flight"
    SENT = "sent"
    ACKED = "acked"
    FAILED = "failed"


@dataclass
class Chunk:
    """Represents a chunk of a file"""
    chunk_id: int
    offset: int
    size: int
    hash: str  # xxHash of chunk data
    status: ChunkStatus = ChunkStatus.PENDING
    retry_count: int = 0
    assigned_link: Optional[str] = None
    sent_at: Optional[datetime] = None
    acked_at: Optional[datetime] = None
    
    def to_dict(self):
        data = asdict(self)
        data['status'] = self.status.value
        data['sent_at'] = self.sent_at.isoformat() if self.sent_at else None
        data['acked_at'] = self.acked_at.isoformat() if self.acked_at else None
        return data
    
    @classmethod
    def from_dict(cls, data: dict):
        data['status'] = ChunkStatus(data['status'])
        if data.get('sent_at'):
            data['sent_at'] = datetime.fromisoformat(data['sent_at'])
        if data.get('acked_at'):
            data['acked_at'] = datetime.fromisoformat(data['acked_at'])
        return cls(**data)


@dataclass
class FileManifest:
    """Manifest for a file transfer"""
    file_id: str
    file_path: str
    file_size: int
    file_hash: str  # SHA-256 of complete file
    total_chunks: int
    chunks: Dict[int, Chunk]
    priority: str = "standard"  # high, standard, background
    created_at: datetime = None
    updated_at: datetime = None
    completed_at: Optional[datetime] = None
    bytes_transferred: int = 0
    bytes_acked: int = 0
    
    def __post_init__(self):
        if self.created_at is None:
            self.created_at = datetime.now()
        if self.updated_at is None:
            self.updated_at = datetime.now()
    
    def to_dict(self):
        data = asdict(self)
        data['created_at'] = self.created_at.isoformat()
        data['updated_at'] = self.updated_at.isoformat()
        data['completed_at'] = self.completed_at.isoformat() if self.completed_at else None
        data['chunks'] = {str(k): v.to_dict() for k, v in self.chunks.items()}
        return data
    
    @classmethod
    def from_dict(cls, data: dict):
        data['created_at'] = datetime.fromisoformat(data['created_at'])
        data['updated_at'] = datetime.fromisoformat(data['updated_at'])
        if data.get('completed_at'):
            data['completed_at'] = datetime.fromisoformat(data['completed_at'])
        data['chunks'] = {int(k): Chunk.from_dict(v) for k, v in data['chunks'].items()}
        return cls(**data)


class ChunkManager:
    """Manages chunks and manifests for file transfers"""
    
    def __init__(self, manifest_dir: Path = MANIFEST_DIR):
        self.manifest_dir = manifest_dir
        self.manifest_dir.mkdir(parents=True, exist_ok=True)
        self.manifests: Dict[str, FileManifest] = {}
    
    def create_manifest(self, file_id: str, file_path: str, file_size: int, 
                       priority: str = "standard") -> FileManifest:
        """Create a new manifest for a file"""
        # Calculate file hash
        file_hash = self._calculate_file_hash(file_path)
        
        # Create chunks
        chunks = {}
        total_chunks = (file_size + CHUNK_SIZE - 1) // CHUNK_SIZE
        
        for i in range(total_chunks):
            offset = i * CHUNK_SIZE
            size = min(CHUNK_SIZE, file_size - offset)
            
            # Calculate chunk hash
            chunk_hash = self._calculate_chunk_hash(file_path, offset, size)
            
            chunks[i] = Chunk(
                chunk_id=i,
                offset=offset,
                size=size,
                hash=chunk_hash
            )
        
        manifest = FileManifest(
            file_id=file_id,
            file_path=file_path,
            file_size=file_size,
            file_hash=file_hash,
            total_chunks=total_chunks,
            chunks=chunks,
            priority=priority
        )
        
        self.manifests[file_id] = manifest
        self._save_manifest(manifest)
        return manifest
    
    def load_manifest(self, file_id: str) -> Optional[FileManifest]:
        """Load a manifest from disk"""
        if file_id in self.manifests:
            return self.manifests[file_id]
        
        manifest_path = self.manifest_dir / f"{file_id}.json"
        if not manifest_path.exists():
            return None
        
        try:
            with open(manifest_path, 'r') as f:
                data = json.load(f)
            manifest = FileManifest.from_dict(data)
            self.manifests[file_id] = manifest
            return manifest
        except Exception as e:
            print(f"Error loading manifest {file_id}: {e}")
            return None
    
    def _save_manifest(self, manifest: FileManifest):
        """Save manifest to disk"""
        manifest_path = self.manifest_dir / f"{manifest.file_id}.json"
        manifest.updated_at = datetime.now()
        
        try:
            with open(manifest_path, 'w') as f:
                json.dump(manifest.to_dict(), f, indent=2)
        except Exception as e:
            print(f"Error saving manifest {manifest.file_id}: {e}")
    
    def update_chunk_status(self, file_id: str, chunk_id: int, status: ChunkStatus,
                           assigned_link: Optional[str] = None):
        """Update status of a chunk"""
        manifest = self.manifests.get(file_id)
        if not manifest:
            manifest = self.load_manifest(file_id)
            if not manifest:
                return
        
        if chunk_id not in manifest.chunks:
            return
        
        chunk = manifest.chunks[chunk_id]
        chunk.status = status
        
        if status == ChunkStatus.IN_FLIGHT:
            chunk.sent_at = datetime.now()
            chunk.assigned_link = assigned_link
        elif status == ChunkStatus.ACKED:
            chunk.acked_at = datetime.now()
            manifest.bytes_acked += chunk.size
        elif status == ChunkStatus.FAILED:
            chunk.retry_count += 1
        
        self._save_manifest(manifest)
    
    def get_pending_chunks(self, file_id: str, limit: int = None) -> List[Chunk]:
        """Get list of pending chunks"""
        manifest = self.manifests.get(file_id)
        if not manifest:
            manifest = self.load_manifest(file_id)
            if not manifest:
                return []
        
        pending = [chunk for chunk in manifest.chunks.values() 
                  if chunk.status in [ChunkStatus.PENDING, ChunkStatus.FAILED]]
        
        # Sort by priority (failed chunks first, then by chunk_id)
        pending.sort(key=lambda x: (x.status != ChunkStatus.FAILED, x.chunk_id))
        
        if limit:
            return pending[:limit]
        return pending
    
    def get_in_flight_chunks(self, file_id: str) -> List[Chunk]:
        """Get chunks currently in flight"""
        manifest = self.manifests.get(file_id)
        if not manifest:
            return []
        
        return [chunk for chunk in manifest.chunks.values() 
                if chunk.status == ChunkStatus.IN_FLIGHT]
    
    def is_complete(self, file_id: str) -> bool:
        """Check if all chunks are ACKed"""
        manifest = self.manifests.get(file_id)
        if not manifest:
            manifest = self.load_manifest(file_id)
            if not manifest:
                return False
        
        return all(chunk.status == ChunkStatus.ACKED 
                  for chunk in manifest.chunks.values())
    
    def get_progress(self, file_id: str) -> Dict:
        """Get transfer progress for a file"""
        manifest = self.manifests.get(file_id)
        if not manifest:
            manifest = self.load_manifest(file_id)
            if not manifest:
                return {"progress": 0.0, "bytes_transferred": 0, "bytes_total": 0}
        
        bytes_acked = sum(chunk.size for chunk in manifest.chunks.values() 
                         if chunk.status == ChunkStatus.ACKED)
        progress = bytes_acked / manifest.file_size if manifest.file_size > 0 else 0.0
        
        return {
            "progress": progress,
            "bytes_transferred": bytes_acked,
            "bytes_total": manifest.file_size,
            "chunks_complete": sum(1 for c in manifest.chunks.values() 
                                  if c.status == ChunkStatus.ACKED),
            "chunks_total": manifest.total_chunks
        }
    
    def _calculate_file_hash(self, file_path: str) -> str:
        """Calculate SHA-256 hash of entire file"""
        sha256 = hashlib.sha256()
        with open(file_path, 'rb') as f:
            while True:
                data = f.read(65536)
                if not data:
                    break
                sha256.update(data)
        return sha256.hexdigest()
    
    def _calculate_chunk_hash(self, file_path: str, offset: int, size: int) -> str:
        """Calculate xxHash of a chunk"""
        with open(file_path, 'rb') as f:
            f.seek(offset)
            data = f.read(size)
        return xxhash.xxh64(data).hexdigest()

