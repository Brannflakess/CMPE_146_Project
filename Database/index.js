const admin = require('firebase-admin');
const fs = require('fs');
require('dotenv').config();

// Check if serviceAccountKey.json exists
if (!fs.existsSync('./serviceAccountKey.json')) {
  console.error('❌ Error: serviceAccountKey.json not found!');
  console.error('\n📋 Setup Instructions:');
  console.error('1. Go to Firebase Console: https://console.firebase.google.com');
  console.error('2. Select your project');
  console.error('3. Click ⚙️ (Project Settings) → Service Accounts tab');
  console.error('4. Click "Generate New Private Key"');
  console.error('5. Save the downloaded JSON file as "serviceAccountKey.json" in this directory');
  console.error('\n📁 File should be at: ./Database/serviceAccountKey.json\n');
  process.exit(1);
}

// Initialize Firebase with service account
const serviceAccount = require('./serviceAccountKey.json');

admin.initializeApp({
  credential: admin.credential.cert(serviceAccount),
  databaseURL: process.env.FIREBASE_DATABASE_URL
});

const db = admin.database();

async function writeSquatData() {
  try {
    console.log('🔄 Connecting to Firebase Realtime Database...');
    
    // Test connection by writing timestamp
    const timestamp = Date.now();
    
    // Write a single squat event
    await db.ref('squat_events/test_1').set({
      timestamp: timestamp,
      user_id: 'test_user_001',
      squat_count: 1,
      device_id: 'stm32_esp32_device_1',
      sensor_data: {
        ax: 0.61,
        ay: 98.84,
        az: 0.30
      }
    });
    console.log('✅ Successfully wrote squat event #1');

    // Write multiple test squat events
    const batch = [];
    for (let i = 2; i <= 5; i++) {
      batch.push(
        db.ref(`squat_events/test_${i}`).set({
          timestamp: timestamp + (i * 1000),
          user_id: 'test_user_001',
          squat_count: i,
          device_id: 'stm32_esp32_device_1',
          sensor_data: {
            ax: (Math.random() * 2 - 1).toFixed(2),
            ay: (80 + Math.random() * 30).toFixed(2),
            az: (Math.random() * 1).toFixed(2)
          }
        })
      );
    }
    
    await Promise.all(batch);
    console.log(`✅ Successfully wrote ${batch.length} more squat events`);

    // Write session summary
    await db.ref('sessions/session_001').set({
      user_id: 'test_user_001',
      device_id: 'stm32_esp32_device_1',
      session_start: timestamp,
      session_end: timestamp + 5000,
      total_squats: 5,
      duration_ms: 5000
    });
    console.log('✅ Successfully wrote session summary');

    // Read back the data to verify
    console.log('\n📖 Reading back written data...');
    const snapshot = await db.ref('squat_events').once('value');
    const data = snapshot.val();
    
    console.log('\n📊 Squat Events Data:');
    console.log(JSON.stringify(data, null, 2));

    console.log('\n✨ Firebase test complete!');
    process.exit(0);

  } catch (error) {
    console.error('❌ Error:', error.message);
    if (error.code === 'PERMISSION_DENIED') {
      console.error('   Make sure your Firebase security rules allow writes from your device.');
    }
    process.exit(1);
  }
}

// Run the test
writeSquatData();
