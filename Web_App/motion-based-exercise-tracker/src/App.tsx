import { useEffect, useState } from 'react'
import { onValue, ref } from 'firebase/database'
import { getFirebaseDb, isFirebaseConfigured } from './firebase'
import './App.css'

/** Matches `telemetry/current` from the realtime database (device may omit fields). */
type TelemetryCurrent = {
  mode?: string
  pushup_count?: number
  oled_pushups?: number
  squat_count?: number
  oled_squats?: number
  /** Device or session identifier from firmware (e.g. `stm32`). */
  src?: string
  state?: string
  t_ms?: number
  ax?: number
  ay?: number
  az?: number
  filtered?: number
  raw?: number
}

function numOrDash(v: number | undefined): string {
  if (v === undefined || v === null || Number.isNaN(v)) return '—'
  return String(v)
}

function pushupCount(t: TelemetryCurrent | null): number | undefined {
  if (!t) return undefined
  if (typeof t.pushup_count === 'number') return t.pushup_count
  if (typeof t.oled_pushups === 'number') return t.oled_pushups
  return undefined
}

function squatCount(t: TelemetryCurrent | null): number | undefined {
  if (!t) return undefined
  if (typeof t.squat_count === 'number') return t.squat_count
  if (typeof t.oled_squats === 'number') return t.oled_squats
  return undefined
}

export default function App() {
  const configured = isFirebaseConfigured()
  const [telemetry, setTelemetry] = useState<TelemetryCurrent | null>(null)
  const [telemetryLoaded, setTelemetryLoaded] = useState(false)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    if (!configured) return
    const db = getFirebaseDb()
    if (!db) return
    setError(null)
    const currentRef = ref(db, 'telemetry/current')
    const unsub = onValue(
      currentRef,
      (snap) => {
        setTelemetryLoaded(true)
        const v = snap.val() as TelemetryCurrent | null
        setTelemetry(v && typeof v === 'object' ? v : null)
      },
      (err) => setError(err.message)
    )
    return () => unsub()
  }, [configured])

  const sessionLabel = telemetry?.src?.trim() || '—'
  const mode = telemetry?.mode?.trim() || '—'
  const pushups = pushupCount(telemetry)
  const squats = squatCount(telemetry)

  return (
    <div className="app">
      <header className="app-header">
        <h1 className="app-title">Motion exercise tracker</h1>
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

      {configured && !error && (
        <section className="panel panel-session">
          <div className="panel-session-head">
            <h2 className="panel-title">Current session</h2>
            <span className="live-pill" title="Updates when the database changes">
              Live
            </span>
          </div>
          {!telemetryLoaded && <p className="muted">Loading…</p>}
          {telemetryLoaded && !telemetry && <p className="muted">No data at <code>telemetry/current</code> yet.</p>}
          {telemetryLoaded && telemetry && (
            <>
              <div className="stats stats-session">
                <div className="stat-card">
                  <span className="stat-label">Mode</span>
                  <span className="stat-value">{mode}</span>
                </div>
                <div className="stat-card">
                  <span className="stat-label">Push-ups</span>
                  <span className="stat-value">{numOrDash(pushups)}</span>
                </div>
                <div className="stat-card">
                  <span className="stat-label">Squats</span>
                  <span className="stat-value">{numOrDash(squats)}</span>
                </div>
              </div>
            </>
          )}
        </section>
      )}
    </div>
  )
}
