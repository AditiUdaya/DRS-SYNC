import React, { useState, useEffect } from 'react'
import axios from 'axios'
import Dashboard from './components/Dashboard'
import Speedometer from './components/Speedometer'
import TelemetryPanel from './components/TelemetryPanel'
import FileUploader from './components/FileUploader'
import './App.css'

const API_BASE = 'http://localhost:8080'

function App() {
  const [transfers, setTransfers] = useState([])
  const [selectedTransfer, setSelectedTransfer] = useState(null)
  const [telemetry, setTelemetry] = useState({
    throughput: 0,
    rtt: 0,
    packetLoss: 0,
    windowSize: 0
  })

  // Poll for transfer status
  useEffect(() => {
    const interval = setInterval(async () => {
      for (const transfer of transfers) {
        try {
          const response = await axios.get(
            `${API_BASE}/transfer/${transfer.file_id}/status`
          )
          
          // Update transfer in list
          setTransfers(prev => prev.map(t => 
            t.file_id === transfer.file_id 
              ? { ...t, ...response.data }
              : t
          ))
          
          // Update telemetry for selected transfer
          if (selectedTransfer?.file_id === transfer.file_id) {
            setTelemetry({
              throughput: response.data.throughput_mbps || 0,
              rtt: 0, // Will be added later
              packetLoss: 0, // Will be calculated
              windowSize: 0 // Will be added later
            })
          }
        } catch (error) {
          console.error('Failed to fetch status:', error)
        }
      }
    }, 1000)

    return () => clearInterval(interval)
  }, [transfers, selectedTransfer])

  const handleCreateTransfer = async (transferData) => {
    try {
      const response = await axios.post(
        `${API_BASE}/manifest/create`,
        transferData
      )
      
      const newTransfer = {
        file_id: response.data.file_id,
        filepath: transferData.filepath,
        priority: transferData.priority,
        status: 'active',
        throughput_mbps: 0,
        chunks_sent: 0,
        chunks_acked: 0
      }
      
      setTransfers(prev => [...prev, newTransfer])
      setSelectedTransfer(newTransfer)
      
    } catch (error) {
      console.error('Failed to create transfer:', error)
      alert('Failed to start transfer. Check console for details.')
    }
  }

  const handlePauseTransfer = async (fileId) => {
    try {
      await axios.post(`${API_BASE}/transfer/${fileId}/pause`)
      setTransfers(prev => prev.map(t => 
        t.file_id === fileId ? { ...t, paused: true } : t
      ))
    } catch (error) {
      console.error('Failed to pause transfer:', error)
    }
  }

  const handleResumeTransfer = async (fileId) => {
    try {
      await axios.post(`${API_BASE}/transfer/${fileId}/resume`)
      setTransfers(prev => prev.map(t => 
        t.file_id === fileId ? { ...t, paused: false } : t
      ))
    } catch (error) {
      console.error('Failed to resume transfer:', error)
    }
  }

  return (
    <div className="app">
      <header className="header">
        <h1>🏎️ DRS-SYNC</h1>
        <p>Formula 1 Inspired File Transfer System</p>
      </header>

      <div className="main-grid">
        <div className="card">
          <FileUploader onCreateTransfer={handleCreateTransfer} />
        </div>

        <div className="card">
          <Speedometer speed={telemetry.throughput} maxSpeed={100} />
        </div>
      </div>

      <div className="main-grid">
        <div className="card">
          <Dashboard
            transfers={transfers}
            selectedTransfer={selectedTransfer}
            onSelectTransfer={setSelectedTransfer}
            onPauseTransfer={handlePauseTransfer}
            onResumeTransfer={handleResumeTransfer}
          />
        </div>

        <div className="card">
          <TelemetryPanel
            telemetry={telemetry}
            transfer={selectedTransfer}
          />
        </div>
      </div>
    </div>
  )
}

export default App