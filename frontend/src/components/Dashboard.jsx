import React from 'react'

function Dashboard({ transfers, selectedTransfer, onSelectTransfer, onPauseTransfer, onResumeTransfer }) {
  const getPriorityBadge = (priority) => {
    const badges = {
      NORMAL: <span className="status-badge normal">Normal</span>,
      HIGH: <span className="status-badge high">High</span>,
      CRITICAL: <span className="status-badge critical">Critical</span>
    }
    return badges[priority] || badges.NORMAL
  }

  const getProgress = (transfer) => {
    if (!transfer.chunks_sent) return 0
    return ((transfer.chunks_acked / transfer.chunks_sent) * 100).toFixed(1)
  }

  return (
    <div>
      <h2>📊 Active Transfers</h2>
      
      {transfers.length === 0 ? (
        <div className="loading">
          No active transfers. Start one to begin!
        </div>
      ) : (
        <div>
          {transfers.map(transfer => (
            <div
              key={transfer.file_id}
              onClick={() => onSelectTransfer(transfer)}
              style={{
                padding: '1rem',
                marginBottom: '1rem',
                background: selectedTransfer?.file_id === transfer.file_id
                  ? 'rgba(225, 6, 0, 0.2)'
                  : 'rgba(255, 255, 255, 0.05)',
                border: '1px solid rgba(255, 255, 255, 0.1)',
                borderRadius: '8px',
                cursor: 'pointer',
                transition: 'all 0.3s ease'
              }}
            >
              <div style={{
                display: 'flex',
                justifyContent: 'space-between',
                alignItems: 'center',
                marginBottom: '0.5rem'
              }}>
                <div>
                  <strong>{transfer.filepath?.split('/').pop() || transfer.file_id}</strong>
                  <div style={{ fontSize: '0.8rem', color: '#888', marginTop: '0.25rem' }}>
                    {transfer.file_id}
                  </div>
                </div>
                {getPriorityBadge(transfer.priority)}
              </div>

              <div style={{
                display: 'flex',
                gap: '2rem',
                fontSize: '0.9rem',
                color: '#aaa',
                marginBottom: '0.5rem'
              }}>
                <div>
                  <strong style={{ color: '#e10600' }}>
                    {transfer.throughput_mbps?.toFixed(2) || '0.00'}
                  </strong> MB/s
                </div>
                <div>
                  Progress: <strong>{getProgress(transfer)}%</strong>
                </div>
                <div>
                  Chunks: {transfer.chunks_acked || 0}/{transfer.chunks_sent || 0}
                </div>
              </div>

              {/* Progress Bar */}
              <div style={{
                width: '100%',
                height: '4px',
                background: 'rgba(255, 255, 255, 0.1)',
                borderRadius: '2px',
                overflow: 'hidden',
                marginBottom: '0.5rem'
              }}>
                <div style={{
                  width: `${getProgress(transfer)}%`,
                  height: '100%',
                  background: 'linear-gradient(90deg, #e10600, #ff6b00)',
                  transition: 'width 0.3s ease'
                }} />
              </div>

              <div style={{ display: 'flex', gap: '0.5rem' }}>
                {transfer.paused ? (
                  <button
                    onClick={(e) => {
                      e.stopPropagation()
                      onResumeTransfer(transfer.file_id)
                    }}
                    style={{ fontSize: '0.8rem', padding: '0.5rem 1rem' }}
                  >
                    ▶️ Resume
                  </button>
                ) : (
                  <button
                    onClick={(e) => {
                      e.stopPropagation()
                      onPauseTransfer(transfer.file_id)
                    }}
                    style={{ fontSize: '0.8rem', padding: '0.5rem 1rem' }}
                  >
                    ⏸️ Pause
                  </button>
                )}
                
                {transfer.completed && (
                  <span style={{
                    padding: '0.5rem 1rem',
                    background: '#4caf50',
                    borderRadius: '4px',
                    fontSize: '0.8rem'
                  }}>
                    ✓ Completed
                  </span>
                )}
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  )
}

export default Dashboard