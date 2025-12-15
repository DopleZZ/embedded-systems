import { useState, useEffect } from 'react'
import { useAuth } from '../context/AuthContext'
import { statsApi } from '../api/statsApi'
import './StatsPage.css'

function StatsPage() {
  const { userId } = useAuth()
  const [stats, setStats] = useState(null)
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState(null)

  useEffect(() => {
    if (userId) {
      loadStats()
    }
  }, [userId])

  const loadStats = async () => {
    if (!userId) return

    try {
      setLoading(true)
      setError(null)
      const data = await statsApi.getPlayerStats(userId)
      setStats(data)
    } catch (err) {
      console.error('Ошибка загрузки статистики:', err)
      setError('Не удалось загрузить статистику')
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
      <div className="stats-page">
        <div className="loading-container">
          <div className="loading-spinner"></div>
          <p>Загрузка статистики...</p>
        </div>
      </div>
    )
  }

  if (error || !stats) {
    return (
      <div className="stats-page">
        <div className="error-container">
          <p>❌ {error || 'Статистика не найдена'}</p>
        </div>
      </div>
    )
  }

  return (
    <div className="stats-page">
      <header className="stats-header">
        <h1 className="stats-title">📊 Статистика</h1>
        <p className="stats-subtitle">Твои достижения и награды</p>
      </header>

      <div className="stats-content">
        <div className="stats-grid">
          <div className="stat-card">
            <div className="stat-icon">😊</div>
            <div className="stat-info">
              <div className="stat-label">Часов счастья</div>
              <div className="stat-value">
                {stats.totalHappyHours?.toFixed(1) || '0.0'}
              </div>
            </div>
          </div>

          <div className="stat-card">
            <div className="stat-icon">🏆</div>
            <div className="stat-info">
              <div className="stat-label">Побед в батлах</div>
              <div className="stat-value">{stats.moodBattlesWon || 0}</div>
            </div>
          </div>

          <div className="stat-card">
            <div className="stat-icon">💔</div>
            <div className="stat-info">
              <div className="stat-label">Поражений в батлах</div>
              <div className="stat-value">{stats.moodBattlesLost || 0}</div>
            </div>
          </div>

          <div className="stat-card">
            <div className="stat-icon">💧</div>
            <div className="stat-info">
              <div className="stat-label">Заработано капель</div>
              <div className="stat-value">{stats.dropsEarned || 0}</div>
            </div>
          </div>
        </div>

        {stats.lastUpdated && (
          <div className="stats-updated">
            <p>Последнее обновление: {formatDate(stats.lastUpdated)}</p>
          </div>
        )}
      </div>
    </div>
  )
}

export default StatsPage

