import { Routes, Route } from 'react-router-dom'
import { AuthProvider } from './context/AuthContext'
import HomePage from './pages/HomePage'
import PlantDetailPage from './pages/PlantDetailPage'
import './App.css'

function App() {
  return (
    <AuthProvider>
      <div className="App">
        <Routes>
          <Route path="/" element={<HomePage />} />
          <Route path="/plant/:plantId" element={<PlantDetailPage />} />
        </Routes>
      </div>
    </AuthProvider>
  )
}

export default App

