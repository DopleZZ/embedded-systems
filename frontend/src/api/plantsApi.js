import axios from 'axios'

const API_BASE_URL = ''

const api = axios.create({
  baseURL: API_BASE_URL,
  headers: {
    'Content-Type': 'application/json',
  },
  withCredentials: true, // для работы с cookies
})

export const plantsApi = {
  // Получить растение по ID
  getPlantById: async (plantId) => {
    const response = await api.get(`/plants/${plantId}`)
    return response.data
  },

  // Получить список растений пользователя
  getPlantsByOwner: async (ownerName) => {
    const response = await api.get(`/plants/by-owner?ownerName=${encodeURIComponent(ownerName)}`)
    return response.data
  },
}

export const authApi = {
  // Регистрация пользователя
  register: async (userName, password, displayName) => {
    const response = await api.post('/auth/register', {
      userName,
      password,
      displayName,
    })
    return response.data
  },

  // Вход пользователя
  login: async (userName, password) => {
    const response = await api.post('/auth/login', {
      userName,
      password,
    })
    return response.data
  },
}

export default api

