import { Routes, Route, Navigate } from 'react-router-dom'
import { AuthProvider } from './context/AuthContext'
import Navigation from './components/Navigation'
import HomePage from './pages/HomePage'
import PlantDetailPage from './pages/PlantDetailPage'
import FriendsPage from './pages/FriendsPage'
import './App.css'

function ProtectedRoute({ children }) {
  // Пока авторизация не обязательна, но можно добавить проверку позже
  return children
}

function App() {
  return (
    <AuthProvider>
      <div className="App">
        <Navigation />
        <Routes>
          <Route path="/" element={<HomePage />} />
          <Route path="/plant/:plantId" element={<PlantDetailPage />} />
          <Route path="/friends" element={<FriendsPage />} />
        </Routes>
      </div>
    </AuthProvider>
  )
}

export default App

