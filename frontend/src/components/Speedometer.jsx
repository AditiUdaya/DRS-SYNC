import React from 'react'
import { motion } from 'framer-motion'

function Speedometer({ speed, maxSpeed = 100 }) {
  const percentage = Math.min((speed / maxSpeed) * 100, 100)
  const rotation = (percentage / 100) * 270 - 135 // -135° to 135°
  
  const getColor = () => {
    if (speed > maxSpeed * 0.7) return '#4caf50' // Green
    if (speed > maxSpeed * 0.4) return '#ff9800' // Orange
    return '#e10600' // Red
  }

  return (
    <div>
      <h2>⚡ Transfer Speed</h2>
      
      <div style={{
        position: 'relative',
        width: '300px',
        height: '200px',
        margin: '2rem auto'
      }}>
        {/* Speedometer Arc */}
        <svg width="300" height="200" viewBox="0 0 300 200">
          {/* Background Arc */}
          <path
            d="M 30 170 A 120 120 0 0 1 270 170"
            fill="none"
            stroke="rgba(255,255,255,0.1)"
            strokeWidth="20"
            strokeLinecap="round"
          />
          
          {/* Progress Arc */}
          <motion.path
            d="M 30 170 A 120 120 0 0 1 270 170"
            fill="none"
            stroke={getColor()}
            strokeWidth="20"
            strokeLinecap="round"
            strokeDasharray="424"
            strokeDashoffset={424 - (424 * percentage / 100)}
            initial={{ strokeDashoffset: 424 }}
            animate={{ strokeDashoffset: 424 - (424 * percentage / 100) }}
            transition={{ duration: 0.5 }}
          />
        </svg>

        {/* Needle */}
        <motion.div
          style={{
            position: 'absolute',
            top: '50%',
            left: '50%',
            width: '4px',
            height: '100px',
            background: getColor(),
            transformOrigin: 'center bottom',
            borderRadius: '2px',
            boxShadow: `0 0 10px ${getColor()}`
          }}
          initial={{ rotate: -135 }}
          animate={{ rotate: rotation }}
          transition={{ type: 'spring', stiffness: 50, damping: 10 }}
        />

        {/* Center Dot */}
        <div style={{
          position: 'absolute',
          top: '50%',
          left: '50%',
          width: '20px',
          height: '20px',
          background: getColor(),
          borderRadius: '50%',
          transform: 'translate(-50%, -50%)',
          boxShadow: `0 0 20px ${getColor()}`
        }} />

        {/* Speed Display */}
        <div style={{
          position: 'absolute',
          bottom: '20px',
          left: '50%',
          transform: 'translateX(-50%)',
          textAlign: 'center'
        }}>
          <div style={{ fontSize: '3rem', fontWeight: 'bold', color: getColor() }}>
            {speed.toFixed(1)}
          </div>
          <div style={{ fontSize: '1rem', color: '#888' }}>
            MB/s
          </div>
        </div>
      </div>
    </div>
  )
}

export default Speedometer