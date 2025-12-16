import api from './plantsApi'

export const gamesApi = {
  // Создать батл настроения
  startMoodBattle: async (challengerPlantId, opponentPlantId, durationHours) => {
    const response = await api.post('/games/mood-battles', {
      challengerPlantId,
      opponentPlantId,
      ...(durationHours && { durationHours }),
    })
    return response.data
  },

  // Получить информацию о батле
  getMoodBattle: async (battleId) => {
    const response = await api.get(`/games/mood-battles/${battleId}`)
    return response.data
  },

  // Получить прогресс недельного челленджа
  getWeeklyChallenge: async (plantId) => {
    const response = await api.get(`/games/wellbeing-challenge?plantId=${plantId}`)
    return response.data
  },
}


