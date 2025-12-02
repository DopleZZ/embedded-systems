import { Link, useLocation } from 'react-router-dom'
import { useAuth } from '../context/AuthContext'
import AuthModal from './AuthModal'
import { useState } from 'react'
import './Navigation.css'

function Navigation() {
  const { isAuthenticated, userName, logout } = useAuth()
  const [showAuthModal, setShowAuthModal] = useState(false)
  const location = useLocation()

  const isActive = (path) => {
    return location.pathname === path
  }

  return (
    <>
      <nav className="navigation">
        <div className="navigation-content">
          <Link to="/" className="navigation-logo">
            🌱 Fitocube
          </Link>

          <div className="navigation-links">
            {isAuthenticated && (
              <>
                <Link
                  to="/"
                  className={`navigation-link ${isActive('/') ? 'active' : ''}`}
                >
                  Растения
                </Link>
                <Link
                  to="/friends"
                  className={`navigation-link ${isActive('/friends') ? 'active' : ''}`}
                >
                  Друзья
                </Link>
              </>
            )}
          </div>

          <div className="navigation-auth">
            {isAuthenticated ? (
              <div className="navigation-user">
                <span className="navigation-username">{userName}</span>
                <button onClick={logout} className="navigation-logout">
                  Выйти
                </button>
              </div>
            ) : (
              <button
                onClick={() => setShowAuthModal(true)}
                className="navigation-login"
              >
                Войти
              </button>
            )}
          </div>
        </div>
      </nav>

      {showAuthModal && (
        <AuthModal onClose={() => setShowAuthModal(false)} />
      )}
    </>
  )
}

export default Navigation

