import React from 'react';
import './TransferCanvas.css';

function TransferCanvas({ files, transferData, eventLog }) {
  const activeTransfers = files.filter(f => f.status === 'active' && transferData[f.file_id]);

  return (
    <div className="transfer-canvas">
      <h2 className="card-title">Live Transfer Canvas</h2>

      {activeTransfers.length === 0 ? (
        <div className="no-transfers">
          <div className="track-visualization">
            <div className="track-lane"></div>
            <div className="track-lane"></div>
            <div className="track-lane"></div>
          </div>
          <p>No active transfers</p>
          <p className="subtext">Upload a file and start a transfer to see it here</p>
        </div>
      ) : (
        <div className="transfers-container">
          {activeTransfers.map((file) => {
            const data = transferData[file.file_id];
            const progress = data?.progress || { progress: 0, bytes_transferred: 0, bytes_total: 0 };
            const stats = data?.stats || {};

            return (
              <div key={file.file_id} className="transfer-lane">
                <div className="lane-header">
                  <span className="lane-name">{file.filename}</span>
                  <span className="lane-speed">
                    {stats.throughput_mbps ? `${stats.throughput_mbps.toFixed(2)} Mbps` : '--'}
                  </span>
                </div>

                <div className="lane-progress-container">
                  <div className="progress-bar large">
                    <div
                      className="progress-fill"
                      style={{ width: `${progress.progress * 100}%` }}
                    >
                      <span className="progress-text">
                        {progress.bytes_transferred > 0
                          ? `${(progress.bytes_transferred / 1024 / 1024).toFixed(2)} MB / ${(progress.bytes_total / 1024 / 1024).toFixed(2)} MB`
                          : '0 MB'}
                      </span>
                    </div>
                  </div>
                </div>

                <div className="lane-stats">
                  <div className="stat-item">
                    <span className="stat-label">Chunks:</span>
                    <span className="stat-value">
                      {progress.chunks_complete || 0} / {progress.chunks_total || 0}
                    </span>
                  </div>
                  <div className="stat-item">
                    <span className="stat-label">Link:</span>
                    <span className="stat-value">{stats.current_link || '--'}</span>
                  </div>
                  <div className="stat-item">
                    <span className="stat-label">Retries:</span>
                    <span className="stat-value">{stats.retransmissions || 0}</span>
                  </div>
                  <div className="stat-item">
                    <span className="stat-label">Switches:</span>
                    <span className="stat-value">{stats.link_switches || 0}</span>
                  </div>
                </div>
              </div>
            );
          })}
        </div>
      )}

      <div className="event-log-section">
        <h3 className="event-log-title">Event Log</h3>
        <div className="event-log">
          {eventLog.length === 0 ? (
            <p className="no-events">No events yet</p>
          ) : (
            eventLog.map((event, idx) => (
              <div key={idx} className={`event-item event-${event.type}`}>
                <span className="event-time">{event.timestamp}</span>
                <span className="event-message">{event.message}</span>
              </div>
            ))
          )}
        </div>
      </div>
    </div>
  );
}

export default TransferCanvas;

