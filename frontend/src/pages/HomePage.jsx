import { useState, useEffect } from 'react'
import { plantsApi } from '../api/plantsApi'
import { useAuth } from '../context/AuthContext'
import PlantCard from '../components/PlantCard'
import './HomePage.css'

function HomePage() {
  const { userName, isAuthenticated } = useAuth()
  const [plants, setPlants] = useState([])
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState(null)

  useEffect(() => {
    if (userName) {
      loadPlants()
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [userName])

  const loadPlants = async () => {
    if (!userName) return
    
    try {
      setLoading(true)
      setError(null)
      const data = await plantsApi.getPlantsByOwner(userName)
      setPlants(Array.isArray(data) ? data : [])
    } catch (err) {
      console.error('Ошибка загрузки растений:', err)
      setError('Не удалось загрузить растения. Проверьте, что бэкенд запущен и вы авторизованы.')
      setPlants([])
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="home-page">
      <header className="home-header">
        <h1 className="home-title">🌱 Fitocube</h1>
        <p className="home-subtitle">Твои растения</p>
        {userName && (
          <p className="home-user">Пользователь: {userName}</p>
        )}
      </header>

      {loading && (
        <div className="loading-container">
          <div className="loading-spinner"></div>
          <p>Загрузка растений...</p>
        </div>
      )}

      {error && (
        <div className="error-container">
          <p>❌ {error}</p>
        </div>
      )}

      {!loading && !error && plants.length === 0 && (
        <div className="empty-container">
          <p>🌿 У вас пока нет растений</p>
        </div>
      )}

      {!loading && !error && plants.length > 0 && (
        <div className="plants-grid">
          {plants.map((plant) => (
            <PlantCard key={plant.plantId} plant={plant} />
          ))}
        </div>
      )}
    </div>
  )
}

export default HomePage

