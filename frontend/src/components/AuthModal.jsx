import { useState } from 'react'
import { authApi } from '../api/plantsApi'
import { useAuth } from '../context/AuthContext'
import './AuthModal.css'

function AuthModal({ onClose }) {
  const [isLogin, setIsLogin] = useState(true)
  const [userName, setUserName] = useState('')
  const [password, setPassword] = useState('')
  const [displayName, setDisplayName] = useState('')
  const [error, setError] = useState('')
  const [loading, setLoading] = useState(false)
  const { login } = useAuth()

  const handleSubmit = async (e) => {
    e.preventDefault()
    setError('')
    setLoading(true)

    try {
      if (isLogin) {
        // Логин - бэкенд возвращает SessionUser с полями: id, userName, displayName
        const sessionUser = await authApi.login(userName, password)
        login({
          userName: sessionUser.userName || userName,
          userId: sessionUser.id, // SessionUser использует 'id'
          displayName: sessionUser.displayName || displayName || userName,
        })
        onClose()
      } else {
        // Регистрация
        const userData = await authApi.register(userName, password, displayName)
        login(userData)
        onClose()
      }
    } catch (err) {
      console.error('Ошибка авторизации:', err)
      setError(
        err.response?.data?.message ||
        (isLogin ? 'Неверный логин или пароль' : 'Ошибка регистрации')
      )
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="auth-modal-overlay" onClick={onClose}>
      <div className="auth-modal" onClick={(e) => e.stopPropagation()}>
        <button className="auth-modal-close" onClick={onClose}>×</button>
        <h2 className="auth-modal-title">
          {isLogin ? '🌱 Вход' : '🌿 Регистрация'}
        </h2>

        <form onSubmit={handleSubmit} className="auth-form">
          <div className="auth-form-group">
            <label htmlFor="userName">Имя пользователя *</label>
            <input
              id="userName"
              type="text"
              value={userName}
              onChange={(e) => setUserName(e.target.value)}
              required
              placeholder="Введите имя пользователя"
            />
          </div>

          {!isLogin && (
            <div className="auth-form-group">
              <label htmlFor="displayName">Отображаемое имя</label>
              <input
                id="displayName"
                type="text"
                value={displayName}
                onChange={(e) => setDisplayName(e.target.value)}
                placeholder="Как вас называть? (необязательно)"
              />
            </div>
          )}

          <div className="auth-form-group">
            <label htmlFor="password">Пароль *</label>
            <input
              id="password"
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              required
              placeholder="Введите пароль"
            />
          </div>

          {error && <div className="auth-error">{error}</div>}

          <button
            type="submit"
            className="auth-submit-button"
            disabled={loading}
          >
            {loading ? 'Загрузка...' : isLogin ? 'Войти' : 'Зарегистрироваться'}
          </button>
        </form>

        <div className="auth-switch">
          {isLogin ? (
            <>
              Нет аккаунта?{' '}
              <button
                type="button"
                className="auth-switch-button"
                onClick={() => setIsLogin(false)}
              >
                Зарегистрироваться
              </button>
            </>
          ) : (
            <>
              Уже есть аккаунт?{' '}
              <button
                type="button"
                className="auth-switch-button"
                onClick={() => setIsLogin(true)}
              >
                Войти
              </button>
            </>
          )}
        </div>
      </div>
    </div>
  )
}

export default AuthModal

