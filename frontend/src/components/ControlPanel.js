import React, { useState } from 'react';
import { api } from '../services/api';
import './ControlPanel.css';

function ControlPanel({ files, onFilesChange, onTransferEvent }) {
  const [simConfig, setSimConfig] = useState({
    packet_loss: 0,
    latency_ms: 0,
    jitter_ms: 0,
    enabled: false
  });
  const [selectedInterface, setSelectedInterface] = useState('');

  const handleStartAll = async () => {
    const queuedFiles = files.filter(f => f.status !== 'active');
    for (const file of queuedFiles) {
      try {
        await api.startTransfer(file.file_id, 'localhost', 9000);
      } catch (error) {
        console.error(`Error starting ${file.file_id}:`, error);
      }
    }
    onFilesChange();
  };

  const handlePauseAll = async () => {
    const activeFiles = files.filter(f => f.status === 'active' && !f.is_paused);
    for (const file of activeFiles) {
      try {
        await api.pauseTransfer(file.file_id);
      } catch (error) {
        console.error(`Error pausing ${file.file_id}:`, error);
      }
    }
    onFilesChange();
  };

  const handleApplySimulator = async () => {
    try {
      await api.setSimulatorConfig({
        ...simConfig,
        interface: selectedInterface || null
      });
      onTransferEvent({
        timestamp: new Date().toLocaleTimeString(),
        message: `Simulator ${simConfig.enabled ? 'enabled' : 'disabled'}: ${simConfig.packet_loss * 100}% loss, ${simConfig.latency_ms}ms latency`,
        type: 'info'
      });
    } catch (error) {
      console.error('Error setting simulator:', error);
    }
  };

  const handleKillLink = async () => {
    try {
      await api.killLink(selectedInterface || null);
      onTransferEvent({
        timestamp: new Date().toLocaleTimeString(),
        message: `Link killed: ${selectedInterface || 'all'}`,
        type: 'warning'
      });
    } catch (error) {
      console.error('Error killing link:', error);
    }
  };

  const handleRestoreLink = async () => {
    try {
      await api.restoreLink(selectedInterface || null);
      onTransferEvent({
        timestamp: new Date().toLocaleTimeString(),
        message: `Link restored: ${selectedInterface || 'all'}`,
        type: 'success'
      });
    } catch (error) {
      console.error('Error restoring link:', error);
    }
  };

  const handleResetSimulator = async () => {
    try {
      await api.resetSimulator(selectedInterface || null);
      setSimConfig({
        packet_loss: 0,
        latency_ms: 0,
        jitter_ms: 0,
        enabled: false
      });
      onTransferEvent({
        timestamp: new Date().toLocaleTimeString(),
        message: 'Simulator reset',
        type: 'info'
      });
    } catch (error) {
      console.error('Error resetting simulator:', error);
    }
  };

  return (
    <div className="control-panel">
      <div className="control-section">
        <h3>Transfer Controls</h3>
        <div className="control-buttons">
          <button onClick={handleStartAll} className="button button-secondary">
            Start All
          </button>
          <button onClick={handlePauseAll} className="button">
            Pause All
          </button>
        </div>
      </div>

      <div className="control-section">
        <h3>Network Simulator</h3>
        <div className="simulator-controls">
          <div className="control-row">
            <label>Packet Loss (%):</label>
            <input
              type="number"
              min="0"
              max="100"
              step="1"
              value={simConfig.packet_loss * 100}
              onChange={(e) => setSimConfig({ ...simConfig, packet_loss: e.target.value / 100 })}
              className="input small"
            />
          </div>

          <div className="control-row">
            <label>Latency (ms):</label>
            <input
              type="number"
              min="0"
              step="10"
              value={simConfig.latency_ms}
              onChange={(e) => setSimConfig({ ...simConfig, latency_ms: parseFloat(e.target.value) })}
              className="input small"
            />
          </div>

          <div className="control-row">
            <label>Jitter (ms):</label>
            <input
              type="number"
              min="0"
              step="5"
              value={simConfig.jitter_ms}
              onChange={(e) => setSimConfig({ ...simConfig, jitter_ms: parseFloat(e.target.value) })}
              className="input small"
            />
          </div>

          <div className="control-row">
            <label>Interface (optional):</label>
            <input
              type="text"
              value={selectedInterface}
              onChange={(e) => setSelectedInterface(e.target.value)}
              placeholder="Leave empty for all"
              className="input small"
            />
          </div>

          <div className="control-row">
            <label>
              <input
                type="checkbox"
                checked={simConfig.enabled}
                onChange={(e) => setSimConfig({ ...simConfig, enabled: e.target.checked })}
              />
              Enable Simulator
            </label>
          </div>

          <div className="control-buttons">
            <button onClick={handleApplySimulator} className="button button-secondary">
              Apply
            </button>
            <button onClick={handleKillLink} className="button" style={{ background: 'var(--error)' }}>
              Kill Link
            </button>
            <button onClick={handleRestoreLink} className="button" style={{ background: 'var(--success)' }}>
              Restore Link
            </button>
            <button onClick={handleResetSimulator} className="button">
              Reset
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}

export default ControlPanel;

