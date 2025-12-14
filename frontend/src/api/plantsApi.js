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
      ...(displayName && { displayName }),
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

export const friendsApi = {
  // Получить список друзей
  getFriends: async (userId) => {
    const response = await api.get(`/friends?userId=${userId}`)
    return response.data
  },

  // Отправить запрос в друзья
  sendFriendRequest: async (requesterId, targetName) => {
    const response = await api.post('/friends/request', {
      requesterId,
      targetName,
    })
    return response.data
  },

  // Получить входящие запросы
  getIncomingRequests: async () => {
    const response = await api.get('/friends/request/incoming')
    return response.data
  },

  // Получить исходящие запросы
  getOutgoingRequests: async () => {
    const response = await api.get('/friends/request/outgoing')
    return response.data
  },

  // Принять запрос в друзья
  acceptFriendRequest: async (requestId) => {
    const response = await api.post(`/friends/request/${requestId}/accept`)
    return response.data
  },

  // Отклонить запрос в друзья
  rejectFriendRequest: async (requestId) => {
    await api.post(`/friends/request/${requestId}/reject`)
  },
}

export default api

