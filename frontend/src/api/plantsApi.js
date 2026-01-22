import axios from 'axios'

const API_BASE_URL = import.meta.env.VITE_API_BASE_URL || '/api'

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
  getPlantsByOwner: async () => {
    const response = await api.get(`/plants/by-owner`)
    return response.data
  },

  // Запустить полив растения
  triggerWatering: async (plantId, durationSeconds) => {
    const requestBody = {}
    if (durationSeconds && durationSeconds >= 1 && durationSeconds <= 600) {
      requestBody.durationSeconds = durationSeconds
    }
    const response = await api.post(`/plants/watering?plantId=${plantId}`, requestBody)
    return response.data
  },

  // Привязать устройство
  claimDevice: async (deviceUid, nickname, friendVisible) => {
    const payload = {
      deviceUid,
      ...(nickname && { nickname }),
    }
    if (friendVisible !== undefined) {
      payload.friendVisible = friendVisible
    }
    const response = await api.post('/plants/claim', payload)
    return response.data
  },

  // Обновить настройки автополива
  updateAutoWatering: async (plantId, autoWateringData) => {
    const response = await api.post(`/plants/${plantId}/auto-watering`, autoWateringData)
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
