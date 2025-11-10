# Testing Auto-Update Feature - Step by Step Guide

## How It Works

**Version Comparison:**
1. Client has hardcoded version (line 28 in `mainwindow.cpp`): `currentVersion("1.0.0")`
2. On startup, client calls: `GET http://localhost:5000/api/version/check`
3. Server returns current version from database
4. Client compares: `if (serverVersion == currentVersion)` then no update
5. If versions are **different**, mandatory update dialog appears

**Important:** The comparison is **exact string match**, not semantic versioning!
- "1.0.0" == "1.0.0" → No update
- "1.0.1" != "1.0.0" → Update triggered
- "1.1.0" != "1.0.0" → Update triggered

---

## Step-by-Step Testing

### Step 1: Check Current Client Version

Open `qt_client/mainwindow.cpp` line 28:
```cpp
currentVersion("1.0.0")  // This is what the client thinks it is
```

**This is the version your CURRENT client executable has.**

### Step 2: Start the Backend Server

```bash
cd backend
python license_server.py
```

Server should be running on `http://localhost:5000`

### Step 3: Log In to Admin Panel

Open browser: `http://localhost:5000/admin/login`

### Step 4: Check What Version is Set as Current

1. Go to **App Updates** in sidebar
2. Look for the version with green "Current" badge
3. Note what version number it is

**If no version is set as current:**
- You'll see: "No Current Version Set" warning
- Update check will return `update_available: false`
- Client won't see any updates

### Step 5: Create a Test Scenario

**Option A: Simple Test (Version 1.0.1)**

1. Your client is version "1.0.0" (hardcoded)
2. Upload version "1.0.1" via admin panel:
   - Click "Upload New Version"
   - Version: `1.0.1`
   - Upload any ZIP file (can be a dummy for testing)
   - Click Upload
3. Set version "1.0.1" as current (green checkmark button)

**Option B: Already Have a Version?**

If you already uploaded "1.0.0" and set it as current:
- Upload version "1.0.1"
- Set "1.0.1" as current
- Now when client checks, it will see 1.0.1 != 1.0.0

### Step 6: Test the API Manually

Open browser or use curl:
```bash
curl http://localhost:5000/api/version/check
```

**Expected Response:**
```json
{
  "update_available": true,
  "version": "1.0.1",
  "file_size": 15728640,
  "release_notes": "Your release notes here",
  "download_url": "http://localhost:5000/api/version/download/1"
}
```

**If you see:**
```json
{
  "update_available": false,
  "message": "No version set"
}
```
→ You need to upload a version and set it as current!

### Step 7: Run the Client

**Build first (if needed):**
```cmd
cd qt_client\build
cmake --build . --config Release
```

**Run the executable:**
```cmd
cd Release
SecureLicenseClient.exe
```

### Step 8: What Should Happen

**If update is available (serverVersion != clientVersion):**
1. ✅ UAC prompt appears (admin rights)
2. ✅ Client window opens
3. ✅ Immediately see dialog: "A mandatory update is required!"
4. ✅ Shows current version (1.0.0) and new version (1.0.1)
5. ✅ User clicks OK
6. ✅ Download starts with progress bar
7. ✅ App restarts automatically

**If NO update appears:**
- Check server is running
- Check version is set as current in admin panel
- Check API response (step 6)
- Check client and server versions are different

---

## Debugging Steps

### 1. Enable Debug Output

The client prints debug messages. Run from command line to see them:

```cmd
cd qt_client\build\Release
SecureLicenseClient.exe
```

Look for console output:
- `"Update check failed: ..."` → Server not reachable
- `"No updates available"` → Server returned `update_available: false`
- `"Already on latest version: 1.0.0"` → Server version matches client version

### 2. Check Server Logs

In the terminal where you ran `python license_server.py`, you should see:
```
127.0.0.1 - - [date] "GET /api/version/check HTTP/1.1" 200 -
```

If you don't see this, the client isn't reaching the server.

### 3. Test API in Browser

Open: `http://localhost:5000/api/version/check`

You should see JSON response with version info.

### 4. Check Database

```bash
cd backend
python

>>> from license_server import app, db, AppVersion
>>> with app.app_context():
...     versions = AppVersion.query.all()
...     for v in versions:
...         print(f"Version: {v.version}, Current: {v.is_current}")
```

This shows all versions in database and which one is current.

---

## Common Issues

### Issue 1: No Update Dialog Appears

**Cause:** Server version == Client version

**Solution:**
- Change one of them
- Either: Upload different version on server (1.0.1)
- Or: Change client code to different version (1.0.2), rebuild

### Issue 2: "No version set" in API Response

**Cause:** No version marked as current in database

**Solution:**
1. Go to admin panel → App Updates
2. Upload a version
3. Click green checkmark to set as current

### Issue 3: Update Check Silently Fails

**Cause:** Server not running or wrong URL

**Solution:**
- Make sure server is running on port 5000
- Check firewall isn't blocking
- Verify URL in mainwindow.cpp line 298:
  ```cpp
  QUrl url("http://localhost:5000/api/version/check");
  ```

### Issue 4: Update Downloads But Doesn't Apply

**Cause:** Update ZIP doesn't contain valid files

**Solution:**
- Make sure ZIP contains actual executable
- Follow guide in AUTO_UPDATE_GUIDE.md for creating ZIP

---

## Quick Test with Dummy File

If you just want to test the UPDATE DETECTION (not actual update):

1. Create a dummy ZIP file:
   ```cmd
   echo test > dummy.txt
   powershell Compress-Archive -Path dummy.txt -DestinationPath test_update.zip
   ```

2. Upload via admin panel:
   - Version: `1.0.1`
   - File: `test_update.zip`
   - Set as current

3. Make sure client is version `1.0.0`

4. Run client → Update dialog should appear!

5. Don't let it download/install (the update will fail since it's a dummy file)
   - Just verify the dialog appears
   - Close the app

---

## Forcing a Specific Test

**To test with your current build:**

1. **Note your current client version:**
   - Check mainwindow.cpp line 28
   - Let's say it's "1.0.0"

2. **Upload TWO versions on server:**
   - First: Upload version "1.0.0" (matches client)
   - Second: Upload version "1.0.1" (different from client)

3. **Test Scenario A - No Update:**
   - Set "1.0.0" as current
   - Run client
   - Should NOT show update dialog
   - Check console: "Already on latest version: 1.0.0"

4. **Test Scenario B - Update Available:**
   - Set "1.0.1" as current
   - Run client
   - SHOULD show update dialog
   - Shows "Current: 1.0.0, New: 1.0.1"

---

## Expected Timeline

**Startup Sequence:**
```
0s  - App starts (UAC prompt)
0s  - Window appears
0s  - checkForUpdates() called
0-2s - HTTP request to server
2s  - If update available → Dialog appears
2s+ - User clicks OK → Download starts
```

**The check is INSTANT on startup.** If you don't see a dialog within 2-3 seconds, no update is available.

---

## Final Checklist

Before testing, verify:

- [ ] Backend server is running (`python license_server.py`)
- [ ] At least one version is uploaded in admin panel
- [ ] A version is set as **current** (green badge)
- [ ] Server version is **different** from client version
- [ ] Client can reach server (try API in browser)
- [ ] Client is built and executable exists

---

## Pro Tip: Version String Format

The comparison is string-based, so these are considered different:
- "1.0.0" != "1.0.1" ✅ (will trigger update)
- "1.0.0" != "1.1.0" ✅ (will trigger update)
- "1.0.0" != "2.0.0" ✅ (will trigger update)
- "1.0.0" != "v1.0.0" ✅ (will trigger update - different strings!)
- "1.0.0" == "1.0.0" ❌ (no update)

Stick to semantic versioning format: `MAJOR.MINOR.PATCH`

---

If you're still having issues, run this diagnostic:

```bash
# In one terminal
cd backend
python license_server.py

# In another terminal
curl http://localhost:5000/api/version/check

# Check the response - does it show update_available: true?
# What version does it show?
# Does it match your client version in mainwindow.cpp?
```

Let me know what you see! 🔍
