import { Routes, Route } from 'react-router-dom'
import { AuthProvider } from './context/AuthContext'
import Navigation from './components/Navigation'
import HomePage from './pages/HomePage'
import PlantDetailPage from './pages/PlantDetailPage'
import FriendsPage from './pages/FriendsPage'
import GamesPage from './pages/GamesPage'
import StatsPage from './pages/StatsPage'
import './App.css'

function App() {
  return (
    <AuthProvider>
      <div className="App">
        <Navigation />
        <Routes>
          <Route path="/" element={<HomePage />} />
          <Route path="/plant/:plantId" element={<PlantDetailPage />} />
          <Route path="/friends" element={<FriendsPage />} />
          <Route path="/games" element={<GamesPage />} />
          <Route path="/stats" element={<StatsPage />} />
        </Routes>
      </div>
    </AuthProvider>
  )
}

export default App

