import { initializeApp, type FirebaseApp } from 'firebase/app'
import { getDatabase, type Database } from 'firebase/database'

const apiKey = import.meta.env.VITE_FIREBASE_API_KEY
const authDomain = import.meta.env.VITE_FIREBASE_AUTH_DOMAIN
const databaseURL = import.meta.env.VITE_FIREBASE_DATABASE_URL
const projectId = import.meta.env.VITE_FIREBASE_PROJECT_ID
const storageBucket = import.meta.env.VITE_FIREBASE_STORAGE_BUCKET
const messagingSenderId = import.meta.env.VITE_FIREBASE_MESSAGING_SENDER_ID
const appId = import.meta.env.VITE_FIREBASE_APP_ID

export function isFirebaseConfigured(): boolean {
  return Boolean(
    apiKey &&
      authDomain &&
      databaseURL &&
      projectId &&
      storageBucket &&
      messagingSenderId &&
      appId
  )
}

let app: FirebaseApp | null = null
let db: Database | null = null

export function getFirebaseDb(): Database | null {
  if (!isFirebaseConfigured()) return null
  if (!app) {
    app = initializeApp({
      apiKey: apiKey!,
      authDomain: authDomain!,
      databaseURL: databaseURL!,
      projectId: projectId!,
      storageBucket: storageBucket!,
      messagingSenderId: messagingSenderId!,
      appId: appId!,
    })
    db = getDatabase(app)
  }
  return db
}
