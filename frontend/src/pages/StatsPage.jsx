import { useEffect, useMemo, useState } from 'react'
import { useAuth } from '../context/AuthContext'
import { statsApi } from '../api/statsApi'
import { plantsApi } from '../api/plantsApi'
import './StatsPage.css'

const RANGE_OPTIONS = [
  { id: '24h', label: '24 часа', hours: 24 },
  { id: '7d', label: '7 дней', hours: 24 * 7 },
  { id: '30d', label: '30 дней', hours: 24 * 30 },
]

const CHART_CONFIG = [
  {
    key: 'airTemperatureC',
    label: 'Температура воздуха',
    unit: '°C',
    icon: '🌡️',
    color: '#f97316',
  },
  {
    key: 'airHumidityPercent',
    label: 'Влажность воздуха',
    unit: '%',
    icon: '💨',
    color: '#38bdf8',
  },
  {
    key: 'soilMoisturePercent',
    label: 'Влажность почвы',
    unit: '%',
    icon: '💧',
    color: '#22c55e',
  },
]

function StatsPage() {
  const { userId } = useAuth()
  const [plants, setPlants] = useState([])
  const [selectedPlantId, setSelectedPlantId] = useState(null)
  const [range, setRange] = useState('24h')
  const [measurements, setMeasurements] = useState([])
  const [loadingPlants, setLoadingPlants] = useState(true)
  const [loadingMeasurements, setLoadingMeasurements] = useState(false)
  const [error, setError] = useState(null)

  useEffect(() => {
    if (userId) {
      loadPlants()
    }
  }, [userId])

  useEffect(() => {
    if (selectedPlantId) {
      loadMeasurements(selectedPlantId, range)
    }
  }, [selectedPlantId, range])

  const loadPlants = async () => {
    if (!userId) return

    setLoadingPlants(true)
    setError(null)

    try {
      const data = await plantsApi.getPlantsByOwner()
      const list = Array.isArray(data) ? data : []
      setPlants(list)
      if (list.length > 0) {
        setSelectedPlantId((prev) => {
          if (prev && list.some((plant) => plant.plantId === prev)) {
            return prev
          }
          return list[0].plantId
        })
      } else {
        setSelectedPlantId(null)
        setMeasurements([])
      }
    } catch (err) {
      console.error('Ошибка загрузки растений:', err)
      setError('Не удалось загрузить список растений')
    } finally {
      setLoadingPlants(false)
    }
  }

  const loadMeasurements = async (plantId, rangeId) => {
    if (!plantId) return
    const rangeOption = RANGE_OPTIONS.find((option) => option.id === rangeId)
    const hours = rangeOption?.hours || 24
    const now = new Date()
    const from = new Date(now.getTime() - hours * 60 * 60 * 1000)

    setLoadingMeasurements(true)
    setError(null)
    try {
      const data = await statsApi.getPlantMeasurements(
        plantId,
        from.toISOString(),
        now.toISOString()
      )
      setMeasurements(Array.isArray(data) ? data : [])
    } catch (err) {
      console.error('Ошибка загрузки статистики:', err)
      setError('Не удалось загрузить статистику измерений')
    } finally {
      setLoadingMeasurements(false)
    }
  }

  const formatDate = (dateString) => {
    if (!dateString) return '—'
    const date = new Date(dateString)
    return date.toLocaleString('ru-RU', {
      day: '2-digit',
      month: '2-digit',
      year: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
    })
  }

  const selectedPlant = plants.find((plant) => plant.plantId === selectedPlantId)
  const latestMeasurement = measurements[measurements.length - 1] || null

  const chartData = useMemo(() => {
    return CHART_CONFIG.reduce((acc, chart) => {
      acc[chart.key] = measurements
        .map((item) => ({
          value: item[chart.key],
          timestamp: item.timestamp,
        }))
        .filter((item) => item.value !== null && item.value !== undefined)
      return acc
    }, {})
  }, [measurements])

  if (loadingPlants) {
    return (
      <div className="stats-page">
        <div className="loading-container">
          <div className="loading-spinner"></div>
          <p>Загрузка растений...</p>
        </div>
      </div>
    )
  }

  if (error) {
    return (
      <div className="stats-page">
        <div className="error-container">
          <p>❌ {error}</p>
        </div>
      </div>
    )
  }

  return (
    <div className="stats-page">
      <header className="stats-header">
        <h1 className="stats-title">📈 Статистика растений</h1>
        <p className="stats-subtitle">Динамика измерений и состояние растений</p>
      </header>

      <div className="stats-content">
        {plants.length === 0 ? (
          <div className="stats-empty">
            <p>🌿 Нет привязанных растений</p>
          </div>
        ) : (
          <>
            <div className="stats-controls">
              <div className="stats-control">
                <label htmlFor="plant-select">Растение</label>
                <select
                  id="plant-select"
                  value={selectedPlantId ?? ''}
                  onChange={(e) => setSelectedPlantId(Number(e.target.value))}
                >
                  {plants.map((plant) => (
                    <option key={plant.plantId} value={plant.plantId}>
                      {plant.nickname || plant.deviceUid}
                    </option>
                  ))}
                </select>
              </div>

              <div className="stats-control">
                <label>Период</label>
                <div className="stats-range">
                  {RANGE_OPTIONS.map((option) => (
                    <button
                      key={option.id}
                      type="button"
                      className={`stats-range-button ${range === option.id ? 'active' : ''}`}
                      onClick={() => setRange(option.id)}
                    >
                      {option.label}
                    </button>
                  ))}
                </div>
              </div>

              <div className="stats-control stats-meta">
                <div className="stats-meta-title">Выбранное растение</div>
                <div className="stats-meta-value">
                  {selectedPlant?.nickname || selectedPlant?.deviceUid || '—'}
                </div>
                <div className="stats-meta-sub">
                  Обновление: {formatDate(latestMeasurement?.timestamp)}
                </div>
              </div>
            </div>

            {loadingMeasurements ? (
              <div className="loading-container">
                <div className="loading-spinner"></div>
                <p>Загрузка измерений...</p>
              </div>
            ) : measurements.length === 0 ? (
              <div className="stats-empty">
                <p>Нет измерений за выбранный период</p>
              </div>
            ) : (
              <>
                <div className="stats-grid">
                  <div className="stat-card">
                    <div className="stat-icon">🌡️</div>
                    <div className="stat-info">
                      <div className="stat-label">Температура сейчас</div>
                      <div className="stat-value">
                        {latestMeasurement?.airTemperatureC !== null &&
                        latestMeasurement?.airTemperatureC !== undefined
                          ? latestMeasurement.airTemperatureC.toFixed(1)
                          : '—'}
                        °C
                      </div>
                    </div>
                  </div>

                  <div className="stat-card">
                    <div className="stat-icon">💨</div>
                    <div className="stat-info">
                      <div className="stat-label">Влажность воздуха</div>
                      <div className="stat-value">
                        {latestMeasurement?.airHumidityPercent !== null &&
                        latestMeasurement?.airHumidityPercent !== undefined
                          ? latestMeasurement.airHumidityPercent.toFixed(1)
                          : '—'}
                        %
                      </div>
                    </div>
                  </div>

                  <div className="stat-card">
                    <div className="stat-icon">💧</div>
                    <div className="stat-info">
                      <div className="stat-label">Влажность почвы</div>
                      <div className="stat-value">
                        {latestMeasurement?.soilMoisturePercent !== null &&
                        latestMeasurement?.soilMoisturePercent !== undefined
                          ? latestMeasurement.soilMoisturePercent.toFixed(1)
                          : '—'}
                        %
                      </div>
                    </div>
                  </div>
                </div>

                <div className="stats-charts">
                  {CHART_CONFIG.map((chart) => (
                    <ChartCard
                      key={chart.key}
                      title={chart.label}
                      icon={chart.icon}
                      unit={chart.unit}
                      color={chart.color}
                      data={chartData[chart.key] || []}
                    />
                  ))}
                </div>
              </>
            )}
          </>
        )}
      </div>
    </div>
  )
}

function ChartCard({ title, icon, unit, color, data }) {
  const stats = useMemo(() => {
    if (!data.length) {
      return null
    }
    const values = data.map((item) => item.value)
    const min = Math.min(...values)
    const max = Math.max(...values)
    const avg = values.reduce((sum, value) => sum + value, 0) / values.length
    return {
      min,
      max,
      avg,
    }
  }, [data])

  return (
    <div className="chart-card">
      <div className="chart-header">
        <div className="chart-title">
          <span className="chart-icon">{icon}</span>
          <span>{title}</span>
        </div>
        {stats && (
          <div className="chart-stats">
            <span>Мин: {stats.min.toFixed(1)}{unit}</span>
            <span>Ср: {stats.avg.toFixed(1)}{unit}</span>
            <span>Макс: {stats.max.toFixed(1)}{unit}</span>
          </div>
        )}
      </div>

      {data.length < 2 ? (
        <div className="chart-empty">Недостаточно данных для графика</div>
      ) : (
        <LineChart data={data} color={color} />
      )}
    </div>
  )
}

function LineChart({ data, color }) {
  const width = 600
  const height = 220
  const padding = 28
  const values = data.map((item) => item.value)
  const min = Math.min(...values)
  const max = Math.max(...values)
  const range = max - min || 1

  const points = data.map((item, index) => {
    const x =
      data.length === 1
        ? width / 2
        : padding + (index * (width - padding * 2)) / (data.length - 1)
    const y =
      height -
      padding -
      ((item.value - min) / range) * (height - padding * 2)
    return `${x},${y}`
  })

  const gradientId = `chart-gradient-${color.replace('#', '')}`
  const areaPoints = `${points.join(' ')} ${width - padding},${height - padding} ${padding},${height - padding}`

  return (
    <div className="chart-wrapper">
      <svg viewBox={`0 0 ${width} ${height}`} preserveAspectRatio="none">
        <defs>
          <linearGradient id={gradientId} x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stopColor={color} stopOpacity="0.35" />
            <stop offset="100%" stopColor={color} stopOpacity="0" />
          </linearGradient>
        </defs>
        <polygon
          className="chart-area"
          points={areaPoints}
          fill={`url(#${gradientId})`}
          stroke="none"
        />
        <polyline
          className="chart-line"
          points={points.join(' ')}
          fill="none"
          stroke={color}
          strokeWidth="3"
          strokeLinecap="round"
          strokeLinejoin="round"
        />
      </svg>
    </div>
  )
}

export default StatsPage
