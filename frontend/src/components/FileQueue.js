import React, { useState } from 'react';
import { api } from '../services/api';
import './FileQueue.css';

function FileQueue({ files, onFileUploaded, onTransferEvent }) {
  const [selectedFile, setSelectedFile] = useState(null);
  const [uploading, setUploading] = useState(false);
  const [priority, setPriority] = useState('standard');
  const [destHost, setDestHost] = useState('localhost');
  const [destPort, setDestPort] = useState('9000');

  const handleFileSelect = (e) => {
    const file = e.target.files[0];
    if (file) {
      setSelectedFile(file);
    }
  };

  const handleUpload = async () => {
    if (!selectedFile) return;

    setUploading(true);
    try {
      const result = await api.uploadFile(selectedFile, priority);
      onFileUploaded();
      onTransferEvent({
        timestamp: new Date().toLocaleTimeString(),
        file_id: result.file_id,
        message: `File "${result.filename}" queued (${priority} priority)`,
        type: 'success'
      });
      setSelectedFile(null);
      document.getElementById('file-input').value = '';
    } catch (error) {
      console.error('Upload error:', error);
      onTransferEvent({
        timestamp: new Date().toLocaleTimeString(),
        message: `Upload failed: ${error.message}`,
        type: 'error'
      });
    } finally {
      setUploading(false);
    }
  };

  const handleStartTransfer = async (fileId) => {
    try {
      await api.startTransfer(fileId, destHost, parseInt(destPort));
      onTransferEvent({
        timestamp: new Date().toLocaleTimeString(),
        file_id: fileId,
        message: `Transfer started to ${destHost}:${destPort}`,
        type: 'info'
      });
      onFileUploaded();
    } catch (error) {
      console.error('Start transfer error:', error);
    }
  };

  const handlePause = async (fileId) => {
    try {
      await api.pauseTransfer(fileId);
      onFileUploaded();
    } catch (error) {
      console.error('Pause error:', error);
    }
  };

  const handleResume = async (fileId) => {
    try {
      await api.resumeTransfer(fileId);
      onFileUploaded();
    } catch (error) {
      console.error('Resume error:', error);
    }
  };

  const getPriorityColor = (p) => {
    switch (p) {
      case 'high': return 'var(--error)';
      case 'standard': return 'var(--warning)';
      case 'background': return 'var(--text-secondary)';
      default: return 'var(--text-secondary)';
    }
  };

  return (
    <div className="file-queue">
      <h2 className="card-title">File Queue</h2>

      <div className="upload-section">
        <input
          id="file-input"
          type="file"
          onChange={handleFileSelect}
          className="file-input"
        />
        
        <select
          value={priority}
          onChange={(e) => setPriority(e.target.value)}
          className="input"
          style={{ marginTop: '0.5rem' }}
        >
          <option value="high">High Priority</option>
          <option value="standard">Standard Priority</option>
          <option value="background">Background Priority</option>
        </select>

        <button
          onClick={handleUpload}
          disabled={!selectedFile || uploading}
          className="button"
          style={{ marginTop: '0.5rem', width: '100%' }}
        >
          {uploading ? 'Uploading...' : 'Upload File'}
        </button>
      </div>

      <div className="transfer-config">
        <h3 style={{ fontSize: '0.9rem', marginBottom: '0.5rem' }}>Destination</h3>
        <input
          type="text"
          value={destHost}
          onChange={(e) => setDestHost(e.target.value)}
          placeholder="Host"
          className="input"
          style={{ marginBottom: '0.5rem' }}
        />
        <input
          type="number"
          value={destPort}
          onChange={(e) => setDestPort(e.target.value)}
          placeholder="Port"
          className="input"
        />
      </div>

      <div className="files-list">
        {files.length === 0 ? (
          <p style={{ color: 'var(--text-secondary)', textAlign: 'center', padding: '2rem' }}>
            No files in queue
          </p>
        ) : (
          files.map((file) => (
            <div key={file.file_id} className="file-item">
              <div className="file-header">
                <span className="file-name" title={file.filename}>
                  {file.filename}
                </span>
                <span
                  className="priority-badge"
                  style={{ backgroundColor: getPriorityColor(file.priority) }}
                >
                  {file.priority}
                </span>
              </div>
              
              <div className="file-info">
                <span>{(file.size / 1024 / 1024).toFixed(2)} MB</span>
                <span>
                  {file.status === 'active' ? '🟢 Active' : '⚪ Queued'}
                </span>
              </div>

              {file.progress && (
                <div className="progress-bar" style={{ marginTop: '0.5rem' }}>
                  <div
                    className="progress-fill"
                    style={{ width: `${file.progress.progress * 100}%` }}
                  >
                    {(file.progress.progress * 100).toFixed(1)}%
                  </div>
                </div>
              )}

              <div className="file-actions">
                {file.status !== 'active' && (
                  <button
                    onClick={() => handleStartTransfer(file.file_id)}
                    className="button button-secondary"
                    style={{ fontSize: '0.8rem', padding: '0.25rem 0.5rem' }}
                  >
                    Start
                  </button>
                )}
                {file.status === 'active' && (
                  <>
                    {file.is_paused ? (
                      <button
                        onClick={() => handleResume(file.file_id)}
                        className="button button-secondary"
                        style={{ fontSize: '0.8rem', padding: '0.25rem 0.5rem' }}
                      >
                        Resume
                      </button>
                    ) : (
                      <button
                        onClick={() => handlePause(file.file_id)}
                        className="button"
                        style={{ fontSize: '0.8rem', padding: '0.25rem 0.5rem' }}
                      >
                        Pause
                      </button>
                    )}
                  </>
                )}
              </div>
            </div>
          ))
        )}
      </div>
    </div>
  );
}

export default FileQueue;

