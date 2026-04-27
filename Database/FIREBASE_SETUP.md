# Firebase Realtime Database Test - Squat Counter Project

A simple Node.js project to test Firebase Realtime Database connectivity and write squat event data.

## 📋 Prerequisites

- Node.js 14+ installed
- A Firebase project created on [Firebase Console](https://console.firebase.google.com)
- Firebase CLI (optional)

## 🚀 Setup Steps

### 1. Install Dependencies

```bash
npm install
```

### 2. Get Firebase Service Account Key

1. Go to [Firebase Console](https://console.firebase.google.com)
2. Select your project
3. Click the ⚙️ icon (Project Settings)
4. Go to **Service Accounts** tab
5. Click **Generate New Private Key** button
6. Save the downloaded JSON file as `serviceAccountKey.json` in this directory

```
Database/
├── serviceAccountKey.json  ← Place the downloaded JSON file here
├── index.js
├── package.json
└── FIREBASE_SETUP.md
```

### 3. Configure Environment Variables

Copy `.env.example` to `.env` and update with your Firebase database URL:

```bash
cp .env.example .env
```

Edit `.env`:

```
FIREBASE_DATABASE_URL=https://your-project-name.firebaseio.com
```

**Find your database URL:**

- Firebase Console → Realtime Database → Click "Realtime Database" tab
- Look for the URL at the top: `https://your-project-name.firebaseio.com`

### 4. Configure Firebase Security Rules (Important!)

In Firebase Console, go to **Realtime Database → Rules** and set:

```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

⚠️ **Note**: These are public rules for testing only. For production, use proper authentication:

```json
{
  "rules": {
    ".read": "auth != null",
    ".write": "auth != null"
  }
}
```

### 5. Run the Test

```bash
npm start
```

Expected output:

```
🔄 Connecting to Firebase Realtime Database...
✅ Successfully wrote squat event #1
✅ Successfully wrote 4 more squat events
✅ Successfully wrote session summary

📖 Reading back written data...

📊 Squat Events Data:
{
  "test_1": { ... },
  "test_2": { ... },
  ...
}

✨ Firebase test complete!
```

## 📊 Data Structure

The script writes the following JSON structure to Firebase:

```
squat_events/
├── test_1/
│   ├── timestamp: 1682000000000
│   ├── user_id: "test_user_001"
│   ├── squat_count: 1
│   ├── device_id: "stm32_esp32_device_1"
│   └── sensor_data/
│       ├── ax: 0.61
│       ├── ay: 98.84
│       └── az: 0.30
├── test_2/
│   └── ...
└── ...

sessions/
└── session_001/
    ├── user_id: "test_user_001"
    ├── device_id: "stm32_esp32_device_1"
    ├── session_start: timestamp
    ├── session_end: timestamp
    ├── total_squats: 5
    └── duration_ms: 5000
```

## 🔄 Development Mode (Auto-reload)

For development with automatic restart on file changes:

```bash
npm run dev
```

## 🛠️ Troubleshooting

| Issue                                                        | Solution                                                 |
| ------------------------------------------------------------ | -------------------------------------------------------- |
| `Module not found: firebase-admin`                           | Run `npm install`                                        |
| `PERMISSION_DENIED`                                          | Check Firebase security rules allow writes               |
| `ENOENT: no such file or directory 'serviceAccountKey.json'` | Download service account key from Firebase Console       |
| `Cannot find module dotenv`                                  | Run `npm install dotenv`                                 |
| Connection timeout                                           | Check your internet connection and Firebase database URL |

## 📝 Next Steps for ESP32 Integration

Once you're ready to send data from ESP32:

1. Set up Firebase Realtime Database rules to authenticate ESP32
2. Modify the ESP32 code to use Firebase REST API or Firebase SDK
3. Update the database structure paths to match your ESP32 write path
4. Add user authentication and device validation

## 📚 Resources

- [Firebase Admin SDK Documentation](https://firebase.google.com/docs/database/admin/start)
- [Firebase Realtime Database Rules](https://firebase.google.com/docs/rules)
- [Firebase Security Best Practices](https://firebase.google.com/docs/rules/basics)
