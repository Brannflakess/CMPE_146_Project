import { useEffect, useMemo, useState } from 'react'
import { onValue, ref } from 'firebase/database'
import { getFirebaseDb, isFirebaseConfigured } from './firebase'
import './App.css'

type SquatEventRow = {
  id: string
  timestamp?: number
  user_id?: string
  squat_count?: number
  device_id?: string
  sensor_data?: { ax?: number | string; ay?: number | string; az?: number | string }
}

type SessionRow = {
  id: string
  user_id?: string
  device_id?: string
  session_start?: number
  session_end?: number
  total_squats?: number
  duration_ms?: number
}

function formatTime(ts: number | undefined): string {
  if (ts == null || Number.isNaN(ts)) return '—'
  return new Date(ts).toLocaleString()
}

function num(v: number | string | undefined): string {
  if (v === undefined || v === null) return '—'
  return String(v)
}

export default function App() {
  const configured = isFirebaseConfigured()
  const [events, setEvents] = useState<SquatEventRow[]>([])
  const [sessions, setSessions] = useState<SessionRow[]>([])
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    if (!configured) return
    const db = getFirebaseDb()
    if (!db) return
    setError(null)
    const squatRef = ref(db, 'squat_events')
    const sessionsRef = ref(db, 'sessions')
    const unsubSquat = onValue(
      squatRef,
      (snap) => {
        const raw = snap.val() as Record<string, Omit<SquatEventRow, 'id'>> | null
        if (!raw) {
          setEvents([])
          return
        }
        const rows: SquatEventRow[] = Object.entries(raw).map(([id, ev]) => ({
          id,
          ...ev,
        }))
        rows.sort((a, b) => (b.timestamp ?? 0) - (a.timestamp ?? 0))
        setEvents(rows.slice(0, 50))
      },
      (err) => setError(err.message)
    )
    const unsubSessions = onValue(
      sessionsRef,
      (snap) => {
        const raw = snap.val() as Record<string, Omit<SessionRow, 'id'>> | null
        if (!raw) {
          setSessions([])
          return
        }
        const rows: SessionRow[] = Object.entries(raw).map(([id, s]) => ({
          id,
          ...s,
        }))
        rows.sort((a, b) => (b.session_end ?? b.session_start ?? 0) - (a.session_end ?? a.session_start ?? 0))
        setSessions(rows.slice(0, 20))
      },
      (err) => setError(err.message)
    )
    return () => {
      unsubSquat()
      unsubSessions()
    }
  }, [configured])

  const latestCount = useMemo(() => {
    if (events.length === 0) return null
    const max = Math.max(...events.map((e) => e.squat_count ?? 0))
    return max > 0 ? max : null
  }, [events])

  return (
    <div className="app">
      <header className="app-header">
        <h1 className="app-title">Motion exercise tracker</h1>
        <p className="app-sub">Live squat events and sessions from Firebase Realtime Database</p>
      </header>

      {!configured && (
        <section className="panel panel-warn">
          <p>
            Copy <code>.env.example</code> to <code>.env</code> and paste your Firebase web app config (Project
            settings → Your apps → SDK snippet). Restart <code>npm run dev</code> after saving.
          </p>
        </section>
      )}

      {error && (
        <section className="panel panel-error">
          <p>{error}</p>
        </section>
      )}

      {configured && (
        <section className="stats">
          <div className="stat-card">
            <span className="stat-label">Events shown</span>
            <span className="stat-value">{events.length}</span>
          </div>
          <div className="stat-card">
            <span className="stat-label">Latest squat count</span>
            <span className="stat-value">{latestCount ?? '—'}</span>
          </div>
          <div className="stat-card">
            <span className="stat-label">Sessions shown</span>
            <span className="stat-value">{sessions.length}</span>
          </div>
        </section>
      )}

      <section className="panel">
        <h2 className="panel-title">Squat events</h2>
        {configured && events.length === 0 && !error && <p className="muted">No events yet. Run the Database test script or your device.</p>}
        {events.length > 0 && (
          <div className="table-wrap">
            <table className="data-table">
              <thead>
                <tr>
                  <th>Time</th>
                  <th>User</th>
                  <th>Count</th>
                  <th>Device</th>
                  <th>ax</th>
                  <th>ay</th>
                  <th>az</th>
                </tr>
              </thead>
              <tbody>
                {events.map((e) => (
                  <tr key={e.id}>
                    <td>{formatTime(e.timestamp)}</td>
                    <td>{e.user_id ?? '—'}</td>
                    <td>{e.squat_count ?? '—'}</td>
                    <td className="cell-mono">{e.device_id ?? '—'}</td>
                    <td>{num(e.sensor_data?.ax)}</td>
                    <td>{num(e.sensor_data?.ay)}</td>
                    <td>{num(e.sensor_data?.az)}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}
      </section>

      <section className="panel">
        <h2 className="panel-title">Sessions</h2>
        {configured && sessions.length === 0 && !error && <p className="muted">No sessions yet.</p>}
        {sessions.length > 0 && (
          <div className="table-wrap">
            <table className="data-table">
              <thead>
                <tr>
                  <th>Session</th>
                  <th>User</th>
                  <th>Device</th>
                  <th>Total squats</th>
                  <th>Duration</th>
                  <th>Start</th>
                  <th>End</th>
                </tr>
              </thead>
              <tbody>
                {sessions.map((s) => (
                  <tr key={s.id}>
                    <td className="cell-mono">{s.id}</td>
                    <td>{s.user_id ?? '—'}</td>
                    <td className="cell-mono">{s.device_id ?? '—'}</td>
                    <td>{s.total_squats ?? '—'}</td>
                    <td>{s.duration_ms != null ? `${s.duration_ms} ms` : '—'}</td>
                    <td>{formatTime(s.session_start)}</td>
                    <td>{formatTime(s.session_end)}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}
      </section>
    </div>
  )
}
