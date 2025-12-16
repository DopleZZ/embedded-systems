import { useState, useEffect } from 'react'
import { useAuth } from '../context/AuthContext'
import { gamesApi } from '../api/gamesApi'
import { plantsApi } from '../api/plantsApi'
import './GamesPage.css'

function GamesPage() {
  const { userId } = useAuth()
  const [plants, setPlants] = useState([])
  const [activeTab, setActiveTab] = useState('battles') // battles, challenge
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState(null)
  
  // Батлы
  const [battles, setBattles] = useState([])
  const [showBattleModal, setShowBattleModal] = useState(false)
  const [battleForm, setBattleForm] = useState({
    challengerPlantId: '',
    opponentPlantId: '',
    durationHours: 24,
  })

  // Челлендж
  const [challenge, setChallenge] = useState(null)
  const [selectedPlantForChallenge, setSelectedPlantForChallenge] = useState('')

  useEffect(() => {
    loadPlants()
  }, [])

  useEffect(() => {
    if (activeTab === 'challenge' && selectedPlantForChallenge) {
      loadChallenge()
    }
  }, [activeTab, selectedPlantForChallenge])

  const loadPlants = async () => {
    try {
      setLoading(true)
      const data = await plantsApi.getPlantsByOwner()
      setPlants(Array.isArray(data) ? data : [])
    } catch (err) {
      console.error('Ошибка загрузки растений:', err)
      setError('Не удалось загрузить растения')
    } finally {
      setLoading(false)
    }
  }

  const loadChallenge = async () => {
    if (!selectedPlantForChallenge) return
    try {
      setLoading(true)
      const data = await gamesApi.getWeeklyChallenge(selectedPlantForChallenge)
      setChallenge(data)
    } catch (err) {
      console.error('Ошибка загрузки челленджа:', err)
      if (err.response?.status !== 404) {
        setError('Не удалось загрузить челлендж')
      }
      setChallenge(null)
    } finally {
      setLoading(false)
    }
  }

  const handleStartBattle = async (e) => {
    e.preventDefault()
    if (!battleForm.challengerPlantId || !battleForm.opponentPlantId) {
      setError('Выберите оба растения')
      return
    }

    try {
      setLoading(true)
      setError(null)
      await gamesApi.startMoodBattle(
        parseInt(battleForm.challengerPlantId),
        parseInt(battleForm.opponentPlantId),
        battleForm.durationHours
      )
      setShowBattleModal(false)
      setBattleForm({
        challengerPlantId: '',
        opponentPlantId: '',
        durationHours: 24,
      })
      // Можно загрузить список батлов, если будет эндпоинт
    } catch (err) {
      console.error('Ошибка создания батла:', err)
      setError(err.response?.data?.message || 'Не удалось создать батл')
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

  return (
    <div className="games-page">
      <header className="games-header">
        <h1 className="games-title">🎮 Игры</h1>
        <p className="games-subtitle">Батлы настроения и челленджи</p>
      </header>

      <div className="games-tabs">
        <button
          className={`games-tab ${activeTab === 'battles' ? 'active' : ''}`}
          onClick={() => setActiveTab('battles')}
        >
          ⚔️ Батлы настроения
        </button>
        <button
          className={`games-tab ${activeTab === 'challenge' ? 'active' : ''}`}
          onClick={() => setActiveTab('challenge')}
        >
          🌸 Недельный челлендж
        </button>
      </div>

      {error && (
        <div className="games-error">
          <p>❌ {error}</p>
          <button onClick={() => setError(null)}>×</button>
        </div>
      )}

      {activeTab === 'battles' && (
        <div className="games-content">
          <button
            onClick={() => setShowBattleModal(true)}
            className="start-battle-button"
          >
            ➕ Создать батл
          </button>

          {battles.length === 0 && (
            <div className="games-empty">
              <p>⚔️ У вас пока нет активных батлов</p>
            </div>
          )}
        </div>
      )}

      {activeTab === 'challenge' && (
        <div className="games-content">
          <div className="challenge-select">
            <label>Выберите растение:</label>
            <select
              value={selectedPlantForChallenge}
              onChange={(e) => setSelectedPlantForChallenge(e.target.value)}
            >
              <option value="">-- Выберите растение --</option>
              {plants.map((plant) => (
                <option key={plant.plantId} value={plant.plantId}>
                  {plant.nickname || `Растение #${plant.plantId}`}
                </option>
              ))}
            </select>
          </div>

          {loading && (
            <div className="games-loading">
              <div className="loading-spinner"></div>
              <p>Загрузка...</p>
            </div>
          )}

          {!loading && selectedPlantForChallenge && challenge && (
            <div className="challenge-card">
              <h3>🌸 Недельный челлендж "Цветок счастья"</h3>
              <div className="challenge-stats">
                <div className="challenge-stat">
                  <div className="challenge-stat-label">Цель</div>
                  <div className="challenge-stat-value">
                    {challenge.goalHappyHours?.toFixed(1) || '0.0'} часов
                  </div>
                </div>
                <div className="challenge-stat">
                  <div className="challenge-stat-label">Достигнуто</div>
                  <div className="challenge-stat-value">
                    {challenge.achievedHappyHours?.toFixed(1) || '0.0'} часов
                  </div>
                </div>
                <div className="challenge-stat">
                  <div className="challenge-stat-label">Прогресс</div>
                  <div className="challenge-stat-value">
                    {challenge.goalHappyHours
                      ? Math.round(
                          (challenge.achievedHappyHours / challenge.goalHappyHours) * 100
                        )
                      : 0}
                    %
                  </div>
                </div>
                <div className="challenge-stat">
                  <div className="challenge-stat-label">Статус</div>
                  <div className="challenge-stat-value">
                    {challenge.status === 'completed'
                      ? '✅ Завершен'
                      : challenge.status === 'failed'
                      ? '❌ Провален'
                      : '⏳ В процессе'}
                  </div>
                </div>
              </div>
              {challenge.weekStart && (
                <p className="challenge-week">
                  Неделя с: {new Date(challenge.weekStart).toLocaleDateString('ru-RU')}
                </p>
              )}
            </div>
          )}

          {!loading && selectedPlantForChallenge && !challenge && (
            <div className="games-empty">
              <p>🌿 Это растение не участвует в челлендже</p>
            </div>
          )}

          {!selectedPlantForChallenge && (
            <div className="games-empty">
              <p>🌱 Выберите растение для просмотра челленджа</p>
            </div>
          )}
        </div>
      )}

      {showBattleModal && (
        <div className="modal-overlay" onClick={() => setShowBattleModal(false)}>
          <div className="modal-content" onClick={(e) => e.stopPropagation()}>
            <h2>⚔️ Создать батл настроения</h2>
            <form onSubmit={handleStartBattle} className="battle-form">
              <label>
                Ваше растение:
                <select
                  value={battleForm.challengerPlantId}
                  onChange={(e) =>
                    setBattleForm({ ...battleForm, challengerPlantId: e.target.value })
                  }
                  required
                >
                  <option value="">-- Выберите растение --</option>
                  {plants.map((plant) => (
                    <option key={plant.plantId} value={plant.plantId}>
                      {plant.nickname || `Растение #${plant.plantId}`}
                    </option>
                  ))}
                </select>
              </label>
              <label>
                Растение соперника (ID):
                <input
                  type="number"
                  value={battleForm.opponentPlantId}
                  onChange={(e) =>
                    setBattleForm({ ...battleForm, opponentPlantId: e.target.value })
                  }
                  required
                  placeholder="Введите ID растения соперника"
                />
              </label>
              <label>
                Длительность (часов):
                <input
                  type="number"
                  min="1"
                  value={battleForm.durationHours}
                  onChange={(e) =>
                    setBattleForm({
                      ...battleForm,
                      durationHours: parseInt(e.target.value) || 24,
                    })
                  }
                />
              </label>
              <div className="modal-actions">
                <button type="submit" className="save-button" disabled={loading}>
                  {loading ? 'Создание...' : 'Создать батл'}
                </button>
                <button
                  type="button"
                  onClick={() => setShowBattleModal(false)}
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

export default GamesPage


