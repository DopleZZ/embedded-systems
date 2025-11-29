import { useState, useEffect } from 'react'
import { useParams, useNavigate } from 'react-router-dom'
import { plantsApi } from '../api/plantsApi'
import './PlantDetailPage.css'

const moodEmojis = {
  happy: '😊',
  normal: '😐',
  thirsty: '💧',
  dry: '🌵',
  hot: '🔥',
  cold: '❄️',
}

const moodLabels = {
  happy: 'Счастливое',
  normal: 'Нормальное',
  thirsty: 'Хочет пить',
  dry: 'Сухое',
  hot: 'Жарко',
  cold: 'Холодно',
}

function PlantDetailPage() {
  const { plantId } = useParams()
  const navigate = useNavigate()
  const [plant, setPlant] = useState(null)
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState(null)

  useEffect(() => {
    loadPlant()
  }, [plantId])

  const loadPlant = async () => {
    try {
      setLoading(true)
      setError(null)
      const data = await plantsApi.getPlantById(plantId)
      setPlant(data)
    } catch (err) {
      console.error('Ошибка загрузки растения:', err)
      setError('Не удалось загрузить растение')
    } finally {
      setLoading(false)
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

  if (loading) {
    return (
      <div className="plant-detail-page">
        <div className="loading-container">
          <div className="loading-spinner"></div>
          <p>Загрузка растения...</p>
        </div>
      </div>
    )
  }

  if (error || !plant) {
    return (
      <div className="plant-detail-page">
        <div className="error-container">
          <p>❌ {error || 'Растение не найдено'}</p>
          <button onClick={() => navigate('/')} className="back-button">
            ← Вернуться на главную
          </button>
        </div>
      </div>
    )
  }

  const moodEmoji = moodEmojis[plant.mood] || '🌱'
  const moodLabel = moodLabels[plant.mood] || plant.mood

  return (
    <div className="plant-detail-page">
      <button onClick={() => navigate('/')} className="back-button">
        ← Назад к растениям
      </button>

      <div className="plant-detail-card">
        <div className="plant-detail-header">
          <div className="plant-detail-emoji">{moodEmoji}</div>
          <div className="plant-detail-title">
            <h1>{plant.nickname || 'Без имени'}</h1>
            <p className="plant-detail-mood">{moodLabel}</p>
          </div>
        </div>

        <div className="plant-detail-info">
          <div className="info-section">
            <h2>📱 Информация об устройстве</h2>
            <div className="info-grid">
              <div className="info-item">
                <span className="info-label">ID растения:</span>
                <span className="info-value">{plant.plantId}</span>
              </div>
              <div className="info-item">
                <span className="info-label">ID устройства:</span>
                <span className="info-value">{plant.deviceUid}</span>
              </div>
              {plant.owner && (
                <>
                  <div className="info-item">
                    <span className="info-label">Владелец:</span>
                    <span className="info-value">
                      {plant.owner.displayName || plant.owner.userName}
                    </span>
                  </div>
                  <div className="info-item">
                    <span className="info-label">Имя пользователя:</span>
                    <span className="info-value">{plant.owner.userName}</span>
                  </div>
                </>
              )}
              <div className="info-item">
                <span className="info-label">Видно друзьям:</span>
                <span className="info-value">
                  {plant.friendVisible ? '✅ Да' : '❌ Нет'}
                </span>
              </div>
            </div>
          </div>

          {plant.measurements && (
            <div className="info-section">
              <h2>📊 Показатели</h2>
              <div className="measurements-grid">
                <div className="measurement-card">
                  <div className="measurement-icon">🌡️</div>
                  <div className="measurement-content">
                    <div className="measurement-label">Температура воздуха</div>
                    <div className="measurement-value">
                      {plant.measurements.airTemperatureC?.toFixed(1) || '—'}°C
                    </div>
                  </div>
                </div>

                <div className="measurement-card">
                  <div className="measurement-icon">💨</div>
                  <div className="measurement-content">
                    <div className="measurement-label">Влажность воздуха</div>
                    <div className="measurement-value">
                      {plant.measurements.airHumidityPercent?.toFixed(1) || '—'}%
                    </div>
                  </div>
                </div>

                <div className="measurement-card">
                  <div className="measurement-icon">💧</div>
                  <div className="measurement-content">
                    <div className="measurement-label">Влажность почвы</div>
                    <div className="measurement-value">
                      {plant.measurements.soilMoisturePercent?.toFixed(1) || '—'}%
                    </div>
                  </div>
                </div>

                {plant.measurements.soilMoistureRaw !== undefined && (
                  <div className="measurement-card">
                    <div className="measurement-icon">📈</div>
                    <div className="measurement-content">
                      <div className="measurement-label">Влажность почвы (сырое значение)</div>
                      <div className="measurement-value">
                        {plant.measurements.soilMoistureRaw}
                      </div>
                    </div>
                  </div>
                )}

                {plant.measurements.timestamp && (
                  <div className="measurement-card full-width">
                    <div className="measurement-icon">🕐</div>
                    <div className="measurement-content">
                      <div className="measurement-label">Время последнего измерения</div>
                      <div className="measurement-value">
                        {formatDate(plant.measurements.timestamp)}
                      </div>
                    </div>
                  </div>
                )}
              </div>
            </div>
          )}

          {!plant.measurements && (
            <div className="info-section">
              <p className="no-measurements">
                📭 Показатели пока не доступны
              </p>
            </div>
          )}
        </div>
      </div>
    </div>
  )
}

export default PlantDetailPage

