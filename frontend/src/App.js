import React, { useState, useEffect, useRef } from 'react';
import './App.css';
import FileQueue from './components/FileQueue';
import TransferCanvas from './components/TransferCanvas';
import MetricsPanel from './components/MetricsPanel';
import ControlPanel from './components/ControlPanel';
import { connectWebSocket, disconnectWebSocket } from './services/websocket';

function App() {
  const [files, setFiles] = useState([]);
  const [links, setLinks] = useState([]);
  const [transferData, setTransferData] = useState({});
  const [eventLog, setEventLog] = useState([]);
  const wsRef = useRef(null);

  useEffect(() => {
    // Connect WebSocket
    wsRef.current = connectWebSocket((data) => {
      if (data.type === 'link_metrics') {
        setLinks(data.links || []);
      } else if (data.type === 'transfer_progress') {
        setTransferData(prev => ({
          ...prev,
          [data.file_id]: {
            progress: data.progress,
            stats: data.stats
          }
        }));
        
        // Add to event log
        if (data.progress.progress > 0) {
          setEventLog(prev => [{
            timestamp: new Date().toLocaleTimeString(),
            file_id: data.file_id,
            message: `Progress: ${(data.progress.progress * 100).toFixed(1)}%`,
            type: 'info'
          }, ...prev.slice(0, 49)]); // Keep last 50 events
        }
      }
    });

    // Load initial files
    loadFiles();

    return () => {
      disconnectWebSocket(wsRef.current);
    };
  }, []);

  const loadFiles = async () => {
    try {
      const response = await fetch('/api/files');
      const data = await response.json();
      setFiles(data.files || []);
    } catch (error) {
      console.error('Error loading files:', error);
    }
  };

  const handleFileUploaded = () => {
    loadFiles();
  };

  const handleTransferEvent = (event) => {
    setEventLog(prev => [event, ...prev.slice(0, 49)]);
  };

  return (
    <div className="App">
      <header className="app-header">
        <div className="header-content">
          <h1>🚦 DRS-SYNC</h1>
          <p className="subtitle">Intelligent Multi-Uplink File Transfer Engine</p>
        </div>
      </header>

      <div className="dashboard-container">
        <div className="dashboard-left">
          <FileQueue 
            files={files} 
            onFileUploaded={handleFileUploaded}
            onTransferEvent={handleTransferEvent}
          />
        </div>

        <div className="dashboard-center">
          <TransferCanvas 
            files={files}
            transferData={transferData}
            eventLog={eventLog}
          />
        </div>

        <div className="dashboard-right">
          <MetricsPanel links={links} />
        </div>
      </div>

      <footer className="app-footer">
        <ControlPanel 
          files={files}
          onFilesChange={loadFiles}
          onTransferEvent={handleTransferEvent}
        />
      </footer>
    </div>
  );
}

export default App;

