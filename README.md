<<<<<<< Updated upstream
# DRS-SYNC
=======
# ZIP File Transfer - React App

## Setup Instructions

### Step 1: Install Dependencies
```bash
cd frontend
npm install
```

### Step 2: Start Backend Server
From the project root directory:
```bash
python3 backend/server.py
```

The server will:
- Start on `http://0.0.0.0:8000`
- Create an `uploads` directory for uploaded files
- Display server IP information

### Step 3: Find Server IP
On the server machine, run:
```bash
# macOS/Linux:
ifconfig | grep "inet " | grep -v 127.0.0.1

# Windows:
ipconfig
```

### Step 4: Start React Frontend
In the `frontend` directory:
```bash
npm start
```

The app will open at `http://localhost:3000`

### Step 5: Access from Other PCs
On client machines, open browser and navigate to:
```
http://<SERVER_IP>:3000
```

To connect to the correct backend, set the API URL:
```bash
REACT_APP_API_URL=http://<SERVER_IP>:8000 npm start
```

Or edit `frontend/src/App.js` and change:
```javascript
const API_BASE = 'http://<SERVER_IP>:8000';
```

### Step 6: Upload Files
1. Click "Choose File to Upload"
2. Select a ZIP file (or any file)
3. File will be uploaded to the server
4. Uploaded files appear in the "Available Files" list

### Step 7: Download Files
1. Click "Download" button next to any file
2. File will be downloaded to your default download location

### Step 8: Verify Transfer
Check the `uploads` directory on the server:
```bash
ls -lh backend/uploads/
```

## Quick Start (Same Machine)

Terminal 1 (Backend):
```bash
python3 backend/server.py
```

Terminal 2 (Frontend):
```bash
cd frontend
npm start
```

Then open `http://localhost:3000` in your browser.

>>>>>>> Stashed changes
