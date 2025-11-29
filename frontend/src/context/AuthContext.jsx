import { createContext, useContext, useState } from 'react'

const AuthContext = createContext(null)

// Мок пользователя - временно отключена авторизация
const MOCK_USER = {
  userName: 'montana',
  userId: 1,
  displayName: 'Montana',
}

export function AuthProvider({ children }) {
  // Авторизация временно отключена - всегда залогинен мок пользователь
  const [user] = useState(MOCK_USER)
  const [isAuthenticated] = useState(true)

  const login = (userData) => {
    setUser(userData)
    setIsAuthenticated(true)
    localStorage.setItem('fitocube_user', JSON.stringify(userData))
  }

  const logout = () => {
    setUser(null)
    setIsAuthenticated(false)
    localStorage.removeItem('fitocube_user')
  }

  const value = {
    user,
    isAuthenticated,
    login,
    logout,
    userName: user?.userName || null,
    userId: user?.userId || null,
  }

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>
}

export function useAuth() {
  const context = useContext(AuthContext)
  if (!context) {
    throw new Error('useAuth must be used within AuthProvider')
  }
  return context
}

