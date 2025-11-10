# 🔄 Auto-Update System Guide

## Overview

The License Management System now includes a complete auto-update feature that allows you to distribute application updates to clients automatically.

## Features

✅ **Version Management** - Upload and manage multiple versions
✅ **Automatic Updates** - Clients check for updates on startup
✅ **Progress Tracking** - Download progress indicator
✅ **Release Notes** - Show users what's new
✅ **Safe Updates** - Graceful restart after update
✅ **Download Statistics** - Track how many clients downloaded each version

---

## Admin Panel - Version Management

### 1. Access Version Management

Log in to admin panel and click **"App Updates"** in the sidebar.

### 2. Upload a New Version

**Step 1: Prepare the Update Package**

Create a ZIP file with all necessary files:
```
update_v1.0.1.zip
├── SecureLicenseClient.exe  (your updated executable)
├── Qt6Core.dll
├── Qt6Gui.dll
├── Qt6Widgets.dll
├── Qt6Network.dll
└── platforms/
    └── qwindows.dll
```

**Important:**
- Include ALL DLLs and dependencies
- Use the same folder structure as your app
- The ZIP will extract directly to the application directory

**Step 2: Upload via Admin Panel**

1. Go to **App Updates** → **Upload New Version**
2. Enter version number (e.g., `1.0.1`)
3. Select your ZIP file
4. (Optional) Add release notes
5. Click **Upload Version**

**Step 3: Set as Current**

1. Go back to **App Updates** list
2. Find your newly uploaded version
3. Click the green **checkmark button** to set it as current
4. Only the "current" version will be pushed to clients!

---

## Client Behavior

### On Startup

1. Client checks current version (hardcoded in `mainwindow.cpp`)
2. Contacts server API: `/api/version/check`
3. If newer version available, shows update dialog
4. User can choose to install or skip

### Update Process

1. **Download**: Progress dialog shows download percentage
2. **Extract**: ZIP file extracted to app directory
3. **Restart**: App closes and relaunches automatically
4. Files are replaced while app is closed (Windows batch script)

### Seamless Experience

- Update check is silent if no update available
- Users can cancel download at any time
- Failed updates don't break the application
- Users can continue using the app if they decline update

---

## Setting the Client Version

### In Qt Client

Edit `qt_client/mainwindow.cpp` line 28:

```cpp
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentVersion("1.0.0")  // ← CHANGE THIS
    , downloadReply(nullptr)
```

**Version Format:** Use semantic versioning: `MAJOR.MINOR.PATCH`
- `1.0.0` - Initial release
- `1.0.1` - Bug fix
- `1.1.0` - New feature
- `2.0.0` - Major update

### Rebuild After Changing Version

```cmd
cd qt_client\build
cmake --build . --config Release
```

---

## API Endpoints

### Check for Updates

**Endpoint:** `GET /api/version/check`

**Response:**
```json
{
  "update_available": true,
  "version": "1.0.1",
  "file_size": 15728640,
  "release_notes": "- Bug fixes\n- Performance improvements",
  "download_url": "http://localhost:5000/api/version/download/1"
}
```

### Download Update

**Endpoint:** `GET /api/version/download/<version_id>`

**Response:** ZIP file download

---

## Version Management Routes

All routes require admin authentication:

- `GET /admin/versions` - List all versions
- `GET /admin/versions/upload` - Upload form
- `POST /admin/versions/upload` - Handle upload
- `POST /admin/versions/<id>/set-current` - Set as current version
- `POST /admin/versions/<id>/delete` - Delete version

---

## Database Schema

### AppVersion Table

| Column | Type | Description |
|--------|------|-------------|
| id | Integer | Primary key |
| version | String(20) | Version number (e.g., "1.0.1") |
| is_current | Boolean | Only one version should be True |
| filename | String(255) | ZIP filename on server |
| file_size | Integer | File size in bytes |
| release_notes | Text | What's new in this version |
| uploaded_at | DateTime | Upload timestamp |
| uploaded_by | String(80) | Admin username who uploaded |
| download_count | Integer | Number of times downloaded |

---

## Workflow Example

### Scenario: Release Version 1.0.1

**1. Build the New Version**

```cmd
cd qt_client
# Edit mainwindow.cpp - set currentVersion to "1.0.1"

cd build
cmake --build . --config Release

# Files are in: build\Release\
```

**2. Deploy Qt DLLs**

```cmd
cd build\Release
C:\Qt\6.5.3\msvc2019_64\bin\windeployqt.exe SecureLicenseClient.exe
```

**3. Create Update ZIP**

```cmd
# Create update_v1.0.1.zip containing:
# - SecureLicenseClient.exe
# - All Qt DLLs
# - platforms/ folder with qwindows.dll
```

**4. Upload via Admin Panel**

1. Login: `http://localhost:5000/admin/login`
2. Go to: **App Updates**
3. Click: **Upload New Version**
4. Version: `1.0.1`
5. Release Notes: `Bug fixes and improvements`
6. Select `update_v1.0.1.zip`
7. Click: **Upload Version**

**5. Set as Current**

1. Find version `1.0.1` in the list
2. Click the green checkmark button
3. Done! Clients will now auto-update

**6. Client Updates**

- Users running version `1.0.0` will be prompted on next startup
- They'll see release notes and file size
- Download happens automatically if they accept
- App restarts with new version

---

## Troubleshooting

### Update Not Showing for Clients

**Check:**
- Is the version set as "Current" in admin panel?
- Is the client's `currentVersion` older than server version?
- Is the server running and accessible?
- Check server logs for API errors

### Update Download Fails

**Check:**
- File size under 100MB limit?
- ZIP file exists in `backend/updates/` directory?
- Network connectivity between client and server?

### Update Extraction Fails

**Check:**
- ZIP file structure matches app directory structure
- All required DLLs included in ZIP
- Client has write permissions to app directory
- Antivirus not blocking the update process

### App Won't Restart After Update

**Check:**
- Batch script has permissions to execute
- Application path is correct
- No processes holding locks on executable

---

## Security Considerations

### Production Deployment

**1. Use HTTPS**

Update `mainwindow.cpp`:
```cpp
QUrl url("https://yourdomain.com/api/version/check");
```

**2. Add Authentication** (Optional)

Require authentication for version downloads in production.

**3. Code Signing** (Windows)

Sign your executables with a certificate to avoid Windows SmartScreen warnings.

**4. Verify Downloads**

Add SHA-256 checksums to verify download integrity:

```cpp
QString expectedHash = obj["file_hash"].toString();
QByteArray downloadedData = reply->readAll();
QString actualHash = QCryptographicHash::hash(downloadedData, QCryptographicHash::Sha256).toHex();

if (expectedHash != actualHash) {
    // Download corrupted
}
```

---

## Advanced Features

### Multiple Channels

Support beta/stable channels:

```cpp
QString channel = settings->value("update_channel", "stable").toString();
QUrl url(QString("http://localhost:5000/api/version/check?channel=%1").arg(channel));
```

### Rollback

Keep previous version for rollback:

```cpp
// Before extracting update, backup current version
QString backupPath = appDir + "/backup/";
// Copy current files to backup
// If update fails, restore from backup
```

### Silent Updates

Update without user intervention:

```cpp
// In handleUpdateCheckReply, skip dialog and download automatically
if (updateAvailable && settings->value("auto_update", false).toBool()) {
    downloadUpdate(downloadUrl, serverVersion);
}
```

---

## File Locations

**Server:**
- Update files: `backend/updates/`
- Database: `backend/license.db` (AppVersion table)

**Client:**
- Temp updates: `%TEMP%/update_v{version}.zip`
- Update script: `%TEMP%/update.bat`
- Settings: Windows Registry `HKEY_CURRENT_USER\Software\SecureAuth\LicenseClient`

---

## Quick Reference

### Upload New Version
```
Admin Panel → App Updates → Upload New Version
```

### Set Current Version
```
Admin Panel → App Updates → Click green checkmark
```

### Check Client Version
```cpp
// qt_client/mainwindow.cpp line 28
currentVersion("1.0.0")  // Current client version
```

### Test Update Process
1. Set client version to "1.0.0"
2. Upload version "1.0.1" via admin
3. Set "1.0.1" as current
4. Run client - update dialog should appear

---

🎉 **Your auto-update system is ready to keep clients up-to-date automatically!**
