import React, { useState } from 'react'

function FileUploader({ onCreateTransfer }) {
  const [formData, setFormData] = useState({
    filepath: '',
    destination: '127.0.0.1:9090',
    priority: 'NORMAL'
  })

  const handleSubmit = (e) => {
    e.preventDefault()
    
    if (!formData.filepath) {
      alert('Please enter a file path')
      return
    }
    
    onCreateTransfer(formData)
    
    // Reset form
    setFormData({
      ...formData,
      filepath: ''
    })
  }

  return (
    <div>
      <h2>🚀 Start New Transfer</h2>
      
      <form onSubmit={handleSubmit}>
        <div>
          <label style={{ display: 'block', marginBottom: '0.5rem', color: '#aaa' }}>
            File Path
          </label>
          <input
            type="text"
            placeholder="/path/to/file.zip"
            value={formData.filepath}
            onChange={(e) => setFormData({ ...formData, filepath: e.target.value })}
          />
        </div>

        <div>
          <label style={{ display: 'block', marginBottom: '0.5rem', color: '#aaa' }}>
            Destination (host:port)
          </label>
          <input
            type="text"
            value={formData.destination}
            onChange={(e) => setFormData({ ...formData, destination: e.target.value })}
          />
        </div>

        <div>
          <label style={{ display: 'block', marginBottom: '0.5rem', color: '#aaa' }}>
            Priority
          </label>
          <select
            value={formData.priority}
            onChange={(e) => setFormData({ ...formData, priority: e.target.value })}
          >
            <option value="NORMAL">🟢 Normal</option>
            <option value="HIGH">🟡 High</option>
            <option value="CRITICAL">🔴 Critical</option>
          </select>
        </div>

        <button type="submit">
          Start Transfer
        </button>
      </form>
    </div>
  )
}

export default FileUploader