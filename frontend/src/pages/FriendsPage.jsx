import { useState, useEffect } from 'react'
import { useAuth } from '../context/AuthContext'
import { friendsApi } from '../api/plantsApi'
import './FriendsPage.css'

function FriendsPage() {
  const { userId, userName } = useAuth()
  const [activeTab, setActiveTab] = useState('friends') // friends, incoming, outgoing
  const [friends, setFriends] = useState([])
  const [incomingRequests, setIncomingRequests] = useState([])
  const [outgoingRequests, setOutgoingRequests] = useState([])
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState(null)
  const [newFriendName, setNewFriendName] = useState('')
  const [sendingRequest, setSendingRequest] = useState(false)

  useEffect(() => {
    if (userId) {
      loadData()
    }
  }, [userId, activeTab])

  const loadData = async () => {
    if (!userId) return

    setLoading(true)
    setError(null)

    try {
      if (activeTab === 'friends') {
        const data = await friendsApi.getFriends(userId)
        setFriends(Array.isArray(data) ? data : [])
      } else if (activeTab === 'incoming') {
        const data = await friendsApi.getIncomingRequests()
        setIncomingRequests(Array.isArray(data) ? data : [])
      } else if (activeTab === 'outgoing') {
        const data = await friendsApi.getOutgoingRequests()
        setOutgoingRequests(Array.isArray(data) ? data : [])
      }
    } catch (err) {
      console.error('Ошибка загрузки данных:', err)
      setError('Не удалось загрузить данные')
    } finally {
      setLoading(false)
    }
  }

  const handleSendRequest = async (e) => {
    e.preventDefault()
    if (!newFriendName.trim() || !userId) return

    setSendingRequest(true)
    setError(null)

    try {
      await friendsApi.sendFriendRequest(userId, newFriendName.trim())
      setNewFriendName('')
      // Обновляем исходящие запросы
      if (activeTab === 'outgoing') {
        loadData()
      }
    } catch (err) {
      console.error('Ошибка отправки запроса:', err)
      setError(
        err.response?.data?.message || 'Не удалось отправить запрос в друзья'
      )
    } finally {
      setSendingRequest(false)
    }
  }

  const handleAcceptRequest = async (requestId) => {
    try {
      await friendsApi.acceptFriendRequest(requestId)
      loadData()
    } catch (err) {
      console.error('Ошибка принятия запроса:', err)
      setError('Не удалось принять запрос')
    }
  }

  const handleRejectRequest = async (requestId) => {
    try {
      await friendsApi.rejectFriendRequest(requestId)
      loadData()
    } catch (err) {
      console.error('Ошибка отклонения запроса:', err)
      setError('Не удалось отклонить запрос')
    }
  }

  const formatDate = (dateString) => {
    if (!dateString) return ''
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
    <div className="friends-page">
      <header className="friends-header">
        <h1 className="friends-title">👥 Друзья</h1>
        <p className="friends-subtitle">Управление друзьями и запросами</p>
      </header>

      <div className="friends-tabs">
        <button
          className={`friends-tab ${activeTab === 'friends' ? 'active' : ''}`}
          onClick={() => setActiveTab('friends')}
        >
          Мои друзья ({friends.length})
        </button>
        <button
          className={`friends-tab ${activeTab === 'incoming' ? 'active' : ''}`}
          onClick={() => setActiveTab('incoming')}
        >
          Входящие ({incomingRequests.length})
        </button>
        <button
          className={`friends-tab ${activeTab === 'outgoing' ? 'active' : ''}`}
          onClick={() => setActiveTab('outgoing')}
        >
          Исходящие ({outgoingRequests.length})
        </button>
      </div>

      {error && (
        <div className="friends-error">
          <p>❌ {error}</p>
        </div>
      )}

      {activeTab === 'friends' && (
        <div className="friends-content">
          {loading ? (
            <div className="friends-loading">
              <div className="loading-spinner"></div>
              <p>Загрузка друзей...</p>
            </div>
          ) : friends.length === 0 ? (
            <div className="friends-empty">
              <p>🌿 У вас пока нет друзей</p>
            </div>
          ) : (
            <div className="friends-list">
              {friends.map((friend) => (
                <div key={friend.userId} className="friend-card">
                  <div className="friend-info">
                    <h3>{friend.displayName || friend.userName}</h3>
                    <p className="friend-username">@{friend.userName}</p>
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      )}

      {activeTab === 'incoming' && (
        <div className="friends-content">
          {loading ? (
            <div className="friends-loading">
              <div className="loading-spinner"></div>
              <p>Загрузка запросов...</p>
            </div>
          ) : incomingRequests.length === 0 ? (
            <div className="friends-empty">
              <p>📭 Нет входящих запросов</p>
            </div>
          ) : (
            <div className="friends-requests">
              {incomingRequests.map((request) => (
                <div key={request.requestId} className="request-card">
                  <div className="request-info">
                    <h3>{request.requester?.displayName || request.requester?.userName}</h3>
                    <p className="request-username">@{request.requester?.userName}</p>
                    {request.createdAt && (
                      <p className="request-date">
                        {formatDate(request.createdAt)}
                      </p>
                    )}
                  </div>
                  <div className="request-actions">
                    <button
                      className="request-accept"
                      onClick={() => handleAcceptRequest(request.requestId)}
                    >
                      ✅ Принять
                    </button>
                    <button
                      className="request-reject"
                      onClick={() => handleRejectRequest(request.requestId)}
                    >
                      ❌ Отклонить
                    </button>
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      )}

      {activeTab === 'outgoing' && (
        <div className="friends-content">
          <form onSubmit={handleSendRequest} className="add-friend-form">
            <input
              type="text"
              value={newFriendName}
              onChange={(e) => setNewFriendName(e.target.value)}
              placeholder="Введите имя пользователя"
              className="add-friend-input"
            />
            <button
              type="submit"
              disabled={sendingRequest || !newFriendName.trim()}
              className="add-friend-button"
            >
              {sendingRequest ? 'Отправка...' : '➕ Добавить в друзья'}
            </button>
          </form>

          {loading ? (
            <div className="friends-loading">
              <div className="loading-spinner"></div>
              <p>Загрузка запросов...</p>
            </div>
          ) : outgoingRequests.length === 0 ? (
            <div className="friends-empty">
              <p>📤 Нет исходящих запросов</p>
            </div>
          ) : (
            <div className="friends-requests">
              {outgoingRequests.map((request) => (
                <div key={request.requestId} className="request-card">
                  <div className="request-info">
                    <h3>{request.target?.displayName || request.target?.userName}</h3>
                    <p className="request-username">@{request.target?.userName}</p>
                    <p className="request-status">
                      Статус: {request.status === 'pending' ? '⏳ Ожидание' : request.status}
                    </p>
                    {request.createdAt && (
                      <p className="request-date">
                        {formatDate(request.createdAt)}
                      </p>
                    )}
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      )}
    </div>
  )
}

export default FriendsPage

