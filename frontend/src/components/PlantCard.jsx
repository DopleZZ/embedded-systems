import { useNavigate } from 'react-router-dom'
import './PlantCard.css'

const moodEmojis = {
  happy: '😊',
  normal: '😐',
  thirsty: '💧',
  dry: '🌵',
  hot: '🔥',
  cold: '❄️',
}

const moodColors = {
  happy: '#4ade80',
  normal: '#94a3b8',
  thirsty: '#60a5fa',
  dry: '#f59e0b',
  hot: '#f87171',
  cold: '#38bdf8',
}

function PlantCard({ plant }) {
  const navigate = useNavigate()

  const handleClick = () => {
    navigate(`/plant/${plant.plantId}`)
  }

  const moodEmoji = moodEmojis[plant.mood] || '🌱'
  const moodColor = moodColors[plant.mood] || '#94a3b8'

  return (
    <div className="plant-card" onClick={handleClick}>
      <div className="plant-card-header" style={{ backgroundColor: moodColor }}>
        <div className="plant-card-emoji">{moodEmoji}</div>
        <div className="plant-card-mood">{plant.mood}</div>
      </div>
      <div className="plant-card-body">
        <h3 className="plant-card-name">{plant.nickname || 'Без имени'}</h3>
        <p className="plant-card-device">ID: {plant.deviceUid}</p>
        {plant.measurements && (
          <div className="plant-card-stats">
            <div className="stat-item">
              <span className="stat-label">🌡️</span>
              <span className="stat-value">
                {plant.measurements.airTemperatureC?.toFixed(1) || '—'}°C
              </span>
            </div>
            <div className="stat-item">
              <span className="stat-label">💧</span>
              <span className="stat-value">
                {plant.measurements.soilMoisturePercent?.toFixed(1) || '—'}%
              </span>
            </div>
          </div>
        )}
      </div>
    </div>
  )
}

export default PlantCard

