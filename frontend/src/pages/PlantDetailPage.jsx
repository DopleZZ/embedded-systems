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
  const [watering, setWatering] = useState(false)
  const [showWateringModal, setShowWateringModal] = useState(false)
  const [showAutoWateringModal, setShowAutoWateringModal] = useState(false)
  const [wateringForm, setWateringForm] = useState({
    durationSeconds: 10, // По умолчанию 10 секунд
  })
  const [autoWateringForm, setAutoWateringForm] = useState({
    enabled: false,
    thresholdPercent: 30,
    durationSeconds: 10,
    cooldownSeconds: 3600,
  })

  useEffect(() => {
    loadPlant()
  }, [plantId])

  useEffect(() => {
    if (plant) {
      setAutoWateringForm({
        enabled: plant.autoWateringEnabled || false,
        thresholdPercent: plant.autoWateringThresholdPercent || 30,
        durationSeconds: plant.autoWateringDurationSeconds || 10,
        cooldownSeconds: plant.autoWateringCooldownSeconds || 3600,
      })
    }
  }, [plant])

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

  const handleWatering = async (e) => {
    e.preventDefault()
    setWatering(true)
    setError(null)
    try {
      const duration = wateringForm.durationSeconds && wateringForm.durationSeconds > 0
        ? wateringForm.durationSeconds
        : undefined
      await plantsApi.triggerWatering(plantId, duration)
      setShowWateringModal(false)
      // Обновляем данные растения после полива
      setTimeout(() => {
        loadPlant()
      }, 1000)
    } catch (err) {
      console.error('Ошибка полива:', err)
      setError(err.response?.data?.message || 'Не удалось запустить полив')
    } finally {
      setWatering(false)
    }
  }

  const handleSaveAutoWatering = async () => {
    try {
      const updated = await plantsApi.updateAutoWatering(plantId, autoWateringForm)
      setPlant(updated)
      setShowAutoWateringModal(false)
      setError(null)
    } catch (err) {
      console.error('Ошибка сохранения автополива:', err)
      setError(err.response?.data?.message || 'Не удалось сохранить настройки автополива')
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

  const formatDuration = (seconds) => {
    if (!seconds) return '—'
    if (seconds < 60) return `${seconds} сек`
    if (seconds < 3600) return `${Math.floor(seconds / 60)} мин`
    return `${Math.floor(seconds / 3600)} ч ${Math.floor((seconds % 3600) / 60)} мин`
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

  if (error && !plant) {
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

  if (!plant) return null

  const moodEmoji = moodEmojis[plant.mood] || '🌱'
  const moodLabel = moodLabels[plant.mood] || plant.mood

  return (
    <div className="plant-detail-page">
      <button onClick={() => navigate('/')} className="back-button">
        ← Назад к растениям
      </button>

      {error && (
        <div className="plant-error-banner">
          <p>❌ {error}</p>
          <button onClick={() => setError(null)}>×</button>
        </div>
      )}

      <div className="plant-detail-card">
        <div className="plant-detail-header">
          <div className="plant-detail-emoji">{moodEmoji}</div>
          <div className="plant-detail-title">
            <h1>{plant.nickname || 'Без имени'}</h1>
            <p className="plant-detail-mood">{moodEmoji} {moodLabel}</p>
          </div>
        </div>

        <div className="plant-actions">
          <button
            onClick={() => setShowWateringModal(true)}
            disabled={watering}
            className="water-button"
          >
            💧 Полить
          </button>
          <button
            onClick={() => setShowAutoWateringModal(true)}
            className="auto-water-button"
          >
            ⚙️ Автополив
          </button>
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
              {plant.lastWateringAt && (
                <div className="info-item">
                  <span className="info-label">Последний полив:</span>
                  <span className="info-value">{formatDate(plant.lastWateringAt)}</span>
                </div>
              )}
              {plant.autoWateringEnabled && (
                <div className="info-item full-width">
                  <span className="info-label">Автополив:</span>
                  <span className="info-value">
                    ✅ Включен (порог: {plant.autoWateringThresholdPercent}%, 
                    длительность: {formatDuration(plant.autoWateringDurationSeconds)})
                  </span>
                </div>
              )}
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

      {showWateringModal && (
        <div className="modal-overlay" onClick={() => setShowWateringModal(false)}>
          <div className="modal-content" onClick={(e) => e.stopPropagation()}>
            <button className="modal-close" onClick={() => setShowWateringModal(false)}>×</button>
            <h2>💧 Полив растения</h2>
            <form onSubmit={handleWatering} className="modal-form">
              <label>
                Длительность полива (секунды):
                <input
                  type="number"
                  min="1"
                  max="600"
                  value={wateringForm.durationSeconds || ''}
                  onChange={(e) =>
                    setWateringForm({
                      durationSeconds: e.target.value ? parseInt(e.target.value) : undefined,
                    })
                  }
                  placeholder="Оставьте пустым для значения по умолчанию"
                />
                <small style={{ color: '#64748b', fontSize: '0.85rem' }}>
                  Минимум: 1 сек, Максимум: 600 сек (10 минут). Оставьте пустым для значения по умолчанию.
                </small>
              </label>
              <div className="modal-actions">
                <button type="submit" className="save-button" disabled={watering}>
                  {watering ? 'Полив...' : 'Запустить полив'}
                </button>
                <button
                  type="button"
                  onClick={() => setShowWateringModal(false)}
                  className="cancel-button"
                  disabled={watering}
                >
                  Отмена
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {showAutoWateringModal && (
        <div className="modal-overlay" onClick={() => setShowAutoWateringModal(false)}>
          <div className="modal-content" onClick={(e) => e.stopPropagation()}>
            <button className="modal-close" onClick={() => setShowAutoWateringModal(false)}>×</button>
            <h2>⚙️ Настройки автополива</h2>
            <form onSubmit={(e) => { e.preventDefault(); handleSaveAutoWatering(); }} className="modal-form">
              <label>
                <input
                  type="checkbox"
                  checked={autoWateringForm.enabled}
                  onChange={(e) =>
                    setAutoWateringForm({ ...autoWateringForm, enabled: e.target.checked })
                  }
                />
                Включить автополив
              </label>
              {autoWateringForm.enabled && (
                <>
                  <label>
                    Порог влажности почвы (%):
                    <input
                      type="number"
                      min="0"
                      max="100"
                      value={autoWateringForm.thresholdPercent}
                      onChange={(e) =>
                        setAutoWateringForm({
                          ...autoWateringForm,
                          thresholdPercent: parseInt(e.target.value) || 0,
                        })
                      }
                    />
                  </label>
                  <label>
                    Длительность полива (сек):
                    <input
                      type="number"
                      min="1"
                      max="600"
                      value={autoWateringForm.durationSeconds}
                      onChange={(e) =>
                        setAutoWateringForm({
                          ...autoWateringForm,
                          durationSeconds: parseInt(e.target.value) || 1,
                        })
                      }
                    />
                  </label>
                  <label>
                    Интервал между поливами (сек):
                    <input
                      type="number"
                      min="0"
                      max="86400"
                      value={autoWateringForm.cooldownSeconds}
                      onChange={(e) =>
                        setAutoWateringForm({
                          ...autoWateringForm,
                          cooldownSeconds: parseInt(e.target.value) || 0,
                        })
                      }
                    />
                  </label>
                </>
              )}
              <div className="modal-actions">
                <button type="submit" className="save-button">
                  Сохранить
                </button>
                <button
                  type="button"
                  onClick={() => setShowAutoWateringModal(false)}
                  className="cancel-button"
                >
                  Отмена
                </button>
              </div>
            </form>
          </div>
        </div>
      )}
    </div>
  )
}

export default PlantDetailPage
