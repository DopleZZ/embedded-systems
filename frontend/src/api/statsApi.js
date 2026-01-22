import api from './plantsApi'

export const statsApi = {
  // Получить историю измерений растения
  getPlantMeasurements: async (plantId, from, to) => {
    const params = new URLSearchParams()
    if (from) {
      params.append('from', from)
    }
    if (to) {
      params.append('to', to)
    }
    const query = params.toString()
    const response = await api.get(
      `/plants/${plantId}/measurements${query ? `?${query}` : ''}`
    )
    return response.data
  },
}

