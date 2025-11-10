# 🪟 Windows Client - Quick Start Guide

## Building the Client

### Option 1: Using Visual Studio 2022 (Recommended)

Since you have Visual Studio 2022 installed, this is the easiest method:

1. **Open "Developer Command Prompt for VS 2022"**
   - Press Windows key
   - Type "Developer Command Prompt"
   - Run as Administrator (recommended)

2. **Navigate to the client folder**
   ```cmd
   cd C:\path\to\basic_login\client
   ```

3. **Run the build script**
   ```cmd
   build.bat
   ```

4. **Done!** You should see `SecureLoginClient.exe` in the client folder

### Option 2: Manual Build with MSVC

If you prefer to build manually:

```cmd
cd client
cl.exe /EHsc /DUNICODE /D_UNICODE /Fe:SecureLoginClient.exe main.cpp /link winhttp.lib comctl32.lib gdi32.lib gdiplus.lib user32.lib msimg32.lib /MANIFESTINPUT:app.manifest
```

### Option 3: Using CMake

```cmd
cd client
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

---

## Running the Client

### 1. Start the License Server First

The client needs the backend server running:

```bash
cd backend
python license_server.py
```

Server should be running on `http://localhost:5000`

### 2. Launch the Client

```cmd
cd client
SecureLoginClient.exe
```

### 3. Test Login

You need a valid license key to log in:

**Create a license key via web admin:**
1. Open browser: `http://localhost:5000/admin/dashboard`
2. Click "Create New License"
3. Enter username (e.g., "testuser")
4. Select duration (e.g., 30 days)
5. Copy the generated license key

**Or create via API:**
```bash
curl -X POST http://localhost:5000/api/admin/licenses \
  -H "Content-Type: application/json" \
  -d "{\"username\": \"testuser\", \"days\": 30}"
```

**Then in the client:**
- Paste the license key in the input field
- Click "Login"
- On first login, the key will bind to your machine's HWID
- Future logins will validate against the bound HWID

---

## Client Features

### Modern Windows 11 UI
✅ Rounded corners and shadows
✅ Smooth gradient backgrounds
✅ Hover effects on buttons
✅ GDI+ anti-aliasing
✅ Professional animations

### Security Features
✅ HWID detection (automatic)
✅ Secure HTTPS communication (if server uses SSL)
✅ No plaintext key storage
✅ IP address tracking
✅ Login attempt logging

### User Experience
✅ Clean, minimal interface
✅ Clear status messages
✅ Error handling with friendly messages
✅ Fast validation (sub-second response)

---

## Configuration

The client is pre-configured to connect to `localhost:5000`.

If you need to change the server address, edit `client/main.cpp`:

```cpp
// Line 53-54
const wchar_t* SERVER_HOST = L"localhost";
const int SERVER_PORT = 5000;
```

Then rebuild:
```cmd
build.bat
```

---

## Testing Workflow

### Complete Test Scenario:

1. **Start the backend**
   ```bash
   cd backend
   python license_server.py
   ```

2. **Create a test license**
   - Browser: `http://localhost:5000/admin/dashboard`
   - Create license for "testuser" with 30 days
   - Copy the license key

3. **Build the client** (if not already built)
   ```cmd
   cd client
   build.bat
   ```

4. **Run the client**
   ```cmd
   SecureLoginClient.exe
   ```

5. **Login**
   - Paste the license key
   - Click "Login"
   - Should see: "License valid! Access granted."

6. **Verify in admin panel**
   - Go back to browser
   - Click "All Licenses"
   - Find "testuser"
   - Click "View"
   - See the login history with your HWID and IP!

---

## Troubleshooting

### Build Errors

**"cl.exe not found"**
- Make sure you're using "Developer Command Prompt for VS 2022"
- Or run: `"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"`

**Linker errors about missing libraries**
- Ensure Windows SDK is installed
- In Visual Studio Installer, verify "Desktop development with C++" is installed

### Runtime Errors

**"Connection failed"**
- Check if backend server is running: `http://localhost:5000/health`
- Verify firewall isn't blocking port 5000

**"Invalid license key"**
- Key must be exactly as generated (64 characters)
- Check for extra spaces when copying
- Verify the key exists in the database: Check admin panel

**"HWID mismatch" or "License suspended"**
- The key is bound to a different machine
- Go to admin panel → License details → Click "Reset HWID"
- Try logging in again

**No GUI appears**
- Check if antivirus is blocking the executable
- Run as Administrator
- Check Windows Event Viewer for error details

### Network Issues

**Certificate errors (SSL)**
- If your server uses HTTPS with self-signed cert
- The client will show certificate errors
- Use proper SSL certificate or disable SSL verification (development only)

---

## Building for Distribution

### Create a Release Build

```cmd
cd client
cl.exe /EHsc /O2 /DUNICODE /D_UNICODE /Fe:SecureLoginClient.exe main.cpp /link winhttp.lib comctl32.lib gdi32.lib gdiplus.lib user32.lib msimg32.lib /MANIFESTINPUT:app.manifest
```

The `/O2` flag optimizes for speed.

### What to Distribute

When giving the client to users, provide:
- `SecureLoginClient.exe` - The application
- `app.manifest` - Windows manifest (embedded in build)

**DO NOT include:**
- Source code (`main.cpp`)
- Build files (`build.bat`, `CMakeLists.txt`, etc.)

### Reducing File Size

To reduce executable size, use UPX (optional):
```cmd
upx --best SecureLoginClient.exe
```

---

## Advanced: Customization

### Change Window Title

Edit `main.cpp` line ~375:
```cpp
CreateWindowExW(
    WS_EX_LAYERED,
    windowClass.lpszClassName,
    L"Your Custom Title",  // ← Change this
    ...
```

### Change Server Address

Edit `main.cpp` lines 53-54:
```cpp
const wchar_t* SERVER_HOST = L"yourserver.com";
const int SERVER_PORT = 443;  // Use 443 for HTTPS
```

### Change Window Size

Edit `main.cpp` lines in `WM_CREATE`:
```cpp
const int WINDOW_WIDTH = 500;   // ← Change width
const int WINDOW_HEIGHT = 400;  // ← Change height
```

After any changes, rebuild with `build.bat`.

---

## Summary

**Quick Start:**
1. Open Developer Command Prompt for VS 2022
2. `cd path\to\basic_login\client`
3. `build.bat`
4. `SecureLoginClient.exe`

**First Time Setup:**
- Create license in web admin first!
- Key binds to your machine on first login
- Subsequent logins must be from same machine

**Need Help?**
- Check `LICENSE_SYSTEM_GUIDE.md` for complete documentation
- Check `WEB_ADMIN_QUICKSTART.md` for admin panel guide
- Server health: `http://localhost:5000/health`

---

🎉 **You're ready to use your secure login client!**
