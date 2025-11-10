# 🎨 Qt Client - Build Guide for Windows

## Overview

The Qt client is a modern, cross-platform GUI application with:
- ✅ Key saving with obfuscation
- ✅ Auto-login functionality
- ✅ HWID detection
- ✅ Dark theme UI
- ✅ Cross-platform (Windows, Linux, macOS)

---

## Installing Qt on Windows

### Option 1: Qt Online Installer (Recommended)

1. **Download Qt Installer**
   - Go to: https://www.qt.io/download-open-source
   - Click "Download the Qt Online Installer"
   - Download for Windows

2. **Run the Installer**
   - Create a Qt account (free)
   - Login to the installer

3. **Select Components**
   - Select **Qt 6.5.x** (or latest Qt 6)
   - Under Qt 6.5.x, check:
     - ✅ MSVC 2019 64-bit (or MSVC 2022 64-bit)
     - ✅ MinGW 11.2.0 64-bit (optional, for non-VS builds)
   - Under Developer and Designer Tools:
     - ✅ CMake (if you don't have it)
     - ✅ Ninja (optional)
   - Click Next and Install

4. **Installation Path**
   - Default: `C:\Qt\`
   - Remember this path!

### Option 2: Using winget (Windows 11)

```powershell
winget install --id=Kitware.CMake -e
winget search Qt  # See available versions
```

---

## Building the Qt Client

### Method 1: Using CMake GUI (Easiest for Beginners)

1. **Open CMake GUI**
   - Press Windows key
   - Type "CMake"
   - Open "CMake (cmake-gui)"

2. **Configure Paths**
   - **Where is the source code:** `C:\path\to\basic_login\qt_client`
   - **Where to build the binaries:** `C:\path\to\basic_login\qt_client\build`
   - Click "Configure"

3. **Select Generator**
   - Choose "Visual Studio 17 2022" (or your VS version)
   - Platform: x64
   - Click "Finish"

4. **Set Qt Path** (if CMake can't find Qt)
   - If you see red errors about Qt not found
   - Click "Add Entry"
   - Name: `CMAKE_PREFIX_PATH`
   - Type: PATH
   - Value: `C:\Qt\6.5.3\msvc2019_64` (adjust to your Qt installation)
   - Click "Configure" again

5. **Generate**
   - Click "Generate"
   - Click "Open Project" to open in Visual Studio

6. **Build in Visual Studio**
   - Right-click "SecureLicenseClient" project
   - Click "Build"
   - Find executable in: `qt_client\build\Debug\SecureLicenseClient.exe`

### Method 2: Command Line (Advanced)

**Prerequisites:**
- Qt installed
- CMake installed
- Visual Studio 2022 with C++ tools

**Steps:**

1. **Open Developer Command Prompt for VS 2022**

2. **Set Qt Path**
   ```cmd
   set CMAKE_PREFIX_PATH=C:\Qt\6.5.3\msvc2019_64
   ```
   (Adjust path to your Qt installation)

3. **Navigate to qt_client**
   ```cmd
   cd C:\path\to\basic_login\qt_client
   ```

4. **Create build directory**
   ```cmd
   mkdir build
   cd build
   ```

5. **Run CMake**
   ```cmd
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```

6. **Build**
   ```cmd
   cmake --build . --config Release
   ```

7. **Find executable**
   ```cmd
   dir Release\SecureLicenseClient.exe
   ```

### Method 3: Qt Creator (Best for Development)

1. **Open Qt Creator**
   - Press Windows key
   - Type "Qt Creator"
   - Open it

2. **Open Project**
   - File → Open File or Project
   - Navigate to `basic_login\qt_client\CMakeLists.txt`
   - Click "Open"

3. **Configure Kit**
   - Select "Desktop Qt 6.5.3 MSVC2019 64bit" (or your version)
   - Click "Configure Project"

4. **Build**
   - Click the green "Run" button (Ctrl+R)
   - Or Build → Build Project
   - Application will launch automatically!

---

## Running the Qt Client

### 1. Start the License Server

```bash
cd backend
python license_server.py
```

Server should be running on `http://localhost:5000`

### 2. Run the Client

**From Build Directory:**
```cmd
cd qt_client\build\Release
SecureLicenseClient.exe
```

**Or from Qt Creator:**
- Just click the green "Run" button

### 3. Test Login

**Create a license via web admin:**
1. Browser: `http://localhost:5000/admin/dashboard`
2. Create new license for "testuser" with 30 days
3. Copy the license key

**In the Qt Client:**
- Paste license key
- Check "Remember my key (auto-login)" if desired
- Click "Login"
- Success! Key is now bound to your HWID

**On Next Launch:**
- If you checked "Remember", client auto-logs in!

---

## Configuring Server Address

By default, the client connects to `http://localhost:5000`.

To change the server address:

1. **Edit mainwindow.cpp**
   - Find line ~15:
   ```cpp
   MainWindow::MainWindow(QWidget *parent)
       : QMainWindow(parent)
       , ui(new Ui::MainWindow)
       , serverUrl("http://localhost:5000/api/validate")  // ← Change this
   ```

2. **Change to your server:**
   ```cpp
   , serverUrl("https://yourserver.com/api/validate")
   ```

3. **Rebuild:**
   ```cmd
   cmake --build . --config Release
   ```

---

## Qt Client Features

### Key Features

**Key Saving:**
- Keys are obfuscated (XOR + Base64) before storage
- Stored in Windows Registry: `HKEY_CURRENT_USER\Software\SecureLicenseClient`
- Can be cleared via "Clear Saved Key" button

**Auto-Login:**
- On startup, if key is saved, auto-validates
- 500ms delay to show UI first
- Seamless user experience

**HWID Detection:**
- Uses Qt's system info APIs
- Combines: CPU ID, Machine ID, and system UUID
- Consistent across reboots

**Network:**
- HTTP/HTTPS support
- JSON API communication
- SSL verification (for production)
- Timeout: 5 seconds

**UI/UX:**
- Dark theme (modern)
- Password-style key input (hidden characters)
- Status bar feedback
- Hover effects on buttons

---

## Troubleshooting

### Build Errors

**"Qt5_DIR not found" or "Qt6_DIR not found"**
```cmd
set CMAKE_PREFIX_PATH=C:\Qt\6.5.3\msvc2019_64
```
Then re-run CMake.

**"MSVC not found"**
- Install "Desktop development with C++" in Visual Studio Installer
- Or use MinGW: Add `-G "MinGW Makefiles"` to cmake command

**"CMakeLists.txt not found"**
- Make sure you're in `qt_client` directory
- Check that `CMakeLists.txt` exists in the directory

### Runtime Errors

**"Cannot find Qt6Core.dll"**
- Option 1: Copy Qt DLLs to executable directory
- Option 2: Use Qt's `windeployqt` tool:
  ```cmd
  C:\Qt\6.5.3\msvc2019_64\bin\windeployqt.exe SecureLicenseClient.exe
  ```

**"Connection refused"**
- Check backend server is running: `http://localhost:5000/health`
- Verify firewall isn't blocking port 5000

**"SSL handshake failed"**
- Using HTTPS with self-signed cert?
- Disable SSL verification (development only):
  ```cpp
  request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
  ```

**Key not saving**
- Check Windows Registry permissions
- Run as Administrator (one time to create keys)

---

## Deploying the Application

### Creating a Distributable Build

1. **Build Release Version**
   ```cmd
   cmake --build . --config Release
   ```

2. **Copy Executable**
   ```cmd
   copy Release\SecureLicenseClient.exe C:\Deploy\
   ```

3. **Deploy Qt Dependencies**
   ```cmd
   cd C:\Deploy
   C:\Qt\6.5.3\msvc2019_64\bin\windeployqt.exe SecureLicenseClient.exe
   ```

4. **Test on Clean Machine**
   - Copy the `C:\Deploy` folder to a test machine
   - Run `SecureLicenseClient.exe`
   - Should work without Qt installed!

### What windeployqt Does

It automatically copies required DLLs:
- Qt6Core.dll
- Qt6Gui.dll
- Qt6Widgets.dll
- Qt6Network.dll
- Platform plugins
- Style plugins

---

## Comparison: Qt Client vs Win32 Client

| Feature | Win32 Client | Qt Client |
|---------|--------------|-----------|
| Cross-platform | ❌ Windows only | ✅ Win/Linux/macOS |
| UI Framework | Win32 API + GDI+ | Qt Widgets |
| Key Saving | ❌ No | ✅ Yes (obfuscated) |
| Auto-Login | ❌ No | ✅ Yes |
| Theme | Modern Win11 | Dark theme |
| Build Complexity | Simple (cl.exe) | Moderate (CMake + Qt) |
| File Size | ~500 KB | ~5 MB (with Qt DLLs) |
| Development | C++ Win32 | C++ Qt |

**Recommendation:**
- **Win32 Client**: Lightweight, Windows-only, simple deployment
- **Qt Client**: Feature-rich, cross-platform, better UX

---

## Quick Reference

### Build Commands
```cmd
# Setup
cd qt_client
mkdir build && cd build

# Configure
set CMAKE_PREFIX_PATH=C:\Qt\6.5.3\msvc2019_64
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config Release

# Deploy
cd Release
C:\Qt\6.5.3\msvc2019_64\bin\windeployqt.exe SecureLicenseClient.exe
```

### Common Paths
- **Qt Installation**: `C:\Qt\6.5.3\msvc2019_64`
- **windeployqt**: `C:\Qt\6.5.3\msvc2019_64\bin\windeployqt.exe`
- **Executable**: `qt_client\build\Release\SecureLicenseClient.exe`
- **Saved Keys**: Windows Registry `HKEY_CURRENT_USER\Software\SecureLicenseClient`

---

## Additional Resources

- **Qt Documentation**: https://doc.qt.io/
- **CMake Tutorial**: https://cmake.org/cmake/help/latest/guide/tutorial/
- **LICENSE_SYSTEM_GUIDE.md**: Complete system documentation
- **WEB_ADMIN_QUICKSTART.md**: Admin panel guide

---

🎉 **Your Qt client is ready to build and deploy!**
