import { useState } from 'react'
import { plantsApi } from '../api/plantsApi'
import './ClaimDeviceModal.css'

function ClaimDeviceModal({ onClose, onSuccess }) {
  const [deviceUid, setDeviceUid] = useState('')
  const [nickname, setNickname] = useState('')
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState('')

  const handleSubmit = async (e) => {
    e.preventDefault()
    if (!deviceUid.trim()) {
      setError('Введите ID устройства')
      return
    }

    setLoading(true)
    setError('')

    try {
      const plant = await plantsApi.claimDevice(deviceUid.trim(), nickname.trim() || undefined)
      onSuccess(plant)
      onClose()
    } catch (err) {
      console.error('Ошибка привязки устройства:', err)
      setError(
        err.response?.data?.message || 'Не удалось привязать устройство'
      )
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="claim-modal-overlay" onClick={onClose}>
      <div className="claim-modal" onClick={(e) => e.stopPropagation()}>
        <button className="claim-modal-close" onClick={onClose}>×</button>
        <h2 className="claim-modal-title">🔗 Привязать устройство</h2>

        <form onSubmit={handleSubmit} className="claim-form">
          <div className="claim-form-group">
            <label htmlFor="deviceUid">ID устройства *</label>
            <input
              id="deviceUid"
              type="text"
              value={deviceUid}
              onChange={(e) => setDeviceUid(e.target.value)}
              required
              placeholder="Введите уникальный код устройства"
            />
          </div>

          <div className="claim-form-group">
            <label htmlFor="nickname">Имя растения</label>
            <input
              id="nickname"
              type="text"
              value={nickname}
              onChange={(e) => setNickname(e.target.value)}
              placeholder="Как назовете растение? (необязательно)"
            />
          </div>

          {error && <div className="claim-error">{error}</div>}

          <button
            type="submit"
            className="claim-submit-button"
            disabled={loading}
          >
            {loading ? 'Привязка...' : 'Привязать устройство'}
          </button>
        </form>
      </div>
    </div>
  )
}

export default ClaimDeviceModal


