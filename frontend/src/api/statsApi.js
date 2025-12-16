import api from './plantsApi'

export const statsApi = {
  // Получить статистику пользователя
  getPlayerStats: async (userId) => {
    const response = await api.get(`/stats/${userId}`)
    return response.data
  },
}


