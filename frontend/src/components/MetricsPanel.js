import React from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, BarChart, Bar } from 'recharts';
import './MetricsPanel.css';

function MetricsPanel({ links }) {
  const [throughputHistory, setThroughputHistory] = React.useState([]);
  const [rttHistory, setRttHistory] = React.useState([]);

  React.useEffect(() => {
    if (links && links.length > 0) {
      const timestamp = new Date().toLocaleTimeString();
      const totalThroughput = links.reduce((sum, link) => sum + (link.throughput_mbps || 0), 0);
      const avgRtt = links.length > 0 
        ? links.reduce((sum, link) => sum + (link.rtt_ms || 0), 0) / links.length 
        : 0;

      setThroughputHistory(prev => [...prev.slice(-19), { time: timestamp, throughput: totalThroughput }]);
      setRttHistory(prev => [...prev.slice(-19), { time: timestamp, rtt: avgRtt }]);
    }
  }, [links]);

  const getScoreColor = (score) => {
    if (score >= 0.7) return 'var(--success)';
    if (score >= 0.4) return 'var(--warning)';
    return 'var(--error)';
  };

  return (
    <div className="metrics-panel">
      <h2 className="card-title">Live Metrics</h2>

      <div className="links-section">
        <h3 style={{ fontSize: '0.9rem', marginBottom: '0.75rem', color: 'var(--primary-blue)' }}>
          Network Links
        </h3>
        {links.length === 0 ? (
          <p style={{ color: 'var(--text-secondary)', textAlign: 'center', padding: '1rem' }}>
            Scanning for links...
          </p>
        ) : (
          links.map((link) => (
            <div key={link.interface} className="link-card">
              <div className="link-header">
                <span className="link-name">{link.interface}</span>
                <span
                  className="link-status"
                  style={{ color: link.is_active ? 'var(--success)' : 'var(--text-secondary)' }}
                >
                  {link.is_active ? '●' : '○'}
                </span>
              </div>

              <div className="link-metrics">
                <div className="metric-row">
                  <span className="metric-label">Score:</span>
                  <span
                    className="metric-value"
                    style={{ color: getScoreColor(link.link_score) }}
                  >
                    {(link.link_score * 100).toFixed(0)}%
                  </span>
                </div>

                <div className="metric-row">
                  <span className="metric-label">Throughput:</span>
                  <span className="metric-value">{link.throughput_mbps.toFixed(2)} Mbps</span>
                </div>

                <div className="metric-row">
                  <span className="metric-label">RTT:</span>
                  <span className="metric-value">{link.rtt_ms.toFixed(1)} ms</span>
                </div>

                <div className="metric-row">
                  <span className="metric-label">Loss:</span>
                  <span className="metric-value">{(link.packet_loss * 100).toFixed(1)}%</span>
                </div>

                <div className="metric-row">
                  <span className="metric-label">Jitter:</span>
                  <span className="metric-value">{link.jitter_ms.toFixed(1)} ms</span>
                </div>

                <div className="metric-row">
                  <span className="metric-label">IP:</span>
                  <span className="metric-value small">{link.ip_address}</span>
                </div>
              </div>

              <div className="link-score-bar">
                <div
                  className="link-score-fill"
                  style={{
                    width: `${link.link_score * 100}%`,
                    backgroundColor: getScoreColor(link.link_score)
                  }}
                />
              </div>
            </div>
          ))
        )}
      </div>

      <div className="charts-section">
        <h3 style={{ fontSize: '0.9rem', marginBottom: '0.75rem', color: 'var(--primary-blue)' }}>
          Throughput Over Time
        </h3>
        <ResponsiveContainer width="100%" height={150}>
          <LineChart data={throughputHistory}>
            <CartesianGrid strokeDasharray="3 3" stroke="#333" />
            <XAxis dataKey="time" stroke="#aaa" fontSize={10} />
            <YAxis stroke="#aaa" fontSize={10} />
            <Tooltip
              contentStyle={{ background: '#1a1a1a', border: '1px solid #333', color: '#fff' }}
            />
            <Line
              type="monotone"
              dataKey="throughput"
              stroke="var(--primary-blue)"
              strokeWidth={2}
              dot={false}
            />
          </LineChart>
        </ResponsiveContainer>

        <h3 style={{ fontSize: '0.9rem', marginTop: '1rem', marginBottom: '0.75rem', color: 'var(--primary-blue)' }}>
          Average RTT
        </h3>
        <ResponsiveContainer width="100%" height={150}>
          <LineChart data={rttHistory}>
            <CartesianGrid strokeDasharray="3 3" stroke="#333" />
            <XAxis dataKey="time" stroke="#aaa" fontSize={10} />
            <YAxis stroke="#aaa" fontSize={10} />
            <Tooltip
              contentStyle={{ background: '#1a1a1a', border: '1px solid #333', color: '#fff' }}
            />
            <Line
              type="monotone"
              dataKey="rtt"
              stroke="var(--warning)"
              strokeWidth={2}
              dot={false}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}

export default MetricsPanel;

