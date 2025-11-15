import React from 'react'
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts'

function TelemetryPanel({ telemetry, transfer }) {
  // Mock data for the chart
  const [chartData, setChartData] = React.useState([])

  React.useEffect(() => {
    if (telemetry.throughput > 0) {
      setChartData(prev => {
        const newData = [...prev, {
          time: new Date().toLocaleTimeString(),
          throughput: telemetry.throughput
        }]
        // Keep last 20 data points
        return newData.slice(-20)
      })
    }
  }, [telemetry.throughput])

  const getStatusFlag = (value, thresholds) => {
    if (value >= thresholds.good) return '🟢'
    if (value >= thresholds.warning) return '🟡'
    return '🔴'
  }

  return (
    <div>
      <h2>📡 Telemetry</h2>
      
      {!transfer ? (
        <div className="loading">
          Select a transfer to view telemetry
        </div>
      ) : (
        <>
          {/* Metrics Grid */}
          <div style={{
            display: 'grid',
            gridTemplateColumns: '1fr 1fr',
            gap: '1rem',
            marginBottom: '2rem'
          }}>
            <div style={{
              padding: '1rem',
              background: 'rgba(255, 255, 255, 0.05)',
              borderRadius: '8px'
            }}>
              <div style={{ fontSize: '0.8rem', color: '#888', marginBottom: '0.5rem' }}>
                Throughput
              </div>
              <div style={{ fontSize: '1.5rem', fontWeight: 'bold', color: '#e10600' }}>
                {getStatusFlag(telemetry.throughput, { good: 50, warning: 20 })}
                {' '}
                {telemetry.throughput.toFixed(2)} MB/s
              </div>
            </div>

            <div style={{
              padding: '1rem',
              background: 'rgba(255, 255, 255, 0.05)',
              borderRadius: '8px'
            }}>
              <div style={{ fontSize: '0.8rem', color: '#888', marginBottom: '0.5rem' }}>
                RTT
              </div>
              <div style={{ fontSize: '1.5rem', fontWeight: 'bold' }}>
                {getStatusFlag(100 - telemetry.rtt, { good: 50, warning: 20 })}
                {' '}
                {telemetry.rtt.toFixed(0)} ms
              </div>
            </div>

            <div style={{
              padding: '1rem',
              background: 'rgba(255, 255, 255, 0.05)',
              borderRadius: '8px'
            }}>
              <div style={{ fontSize: '0.8rem', color: '#888', marginBottom: '0.5rem' }}>
                Packet Loss
              </div>
              <div style={{ fontSize: '1.5rem', fontWeight: 'bold' }}>
                {getStatusFlag(100 - telemetry.packetLoss, { good: 95, warning: 90 })}
                {' '}
                {telemetry.packetLoss.toFixed(2)}%
              </div>
            </div>

            <div style={{
              padding: '1rem',
              background: 'rgba(255, 255, 255, 0.05)',
              borderRadius: '8px'
            }}>
              <div style={{ fontSize: '0.8rem', color: '#888', marginBottom: '0.5rem' }}>
                Window Size
              </div>
              <div style={{ fontSize: '1.5rem', fontWeight: 'bold' }}>
                {telemetry.windowSize}
              </div>
            </div>
          </div>

          {/* Throughput Chart */}
          <div style={{ marginTop: '2rem' }}>
            <h3 style={{ marginBottom: '1rem', fontSize: '1.2rem' }}>
              Throughput Trend
            </h3>
            <ResponsiveContainer width="100%" height={200}>
              <LineChart data={chartData}>
                <CartesianGrid strokeDasharray="3 3" stroke="rgba(255,255,255,0.1)" />
                <XAxis 
                  dataKey="time" 
                  stroke="#888"
                  tick={{ fontSize: 10 }}
                />
                <YAxis 
                  stroke="#888"
                  tick={{ fontSize: 10 }}
                  label={{ value: 'MB/s', angle: -90, position: 'insideLeft' }}
                />
                <Tooltip 
                  contentStyle={{
                    background: 'rgba(10, 14, 39, 0.95)',
                    border: '1px solid rgba(255, 255, 255, 0.2)',
                    borderRadius: '8px'
                  }}
                />
                <Line 
                  type="monotone" 
                  dataKey="throughput" 
                  stroke="#e10600" 
                  strokeWidth={2}
                  dot={false}
                />
              </LineChart>
            </ResponsiveContainer>
          </div>
        </>
      )}
    </div>
  )
}

export default TelemetryPanel