# Professional License Management System

A complete license management platform with **HWID binding**, **auto-suspension**, **IP tracking**, **Qt GUI**, and **expiration support**.

## 🎯 Features Implemented

### ✅ Backend (`license_server.py`)
- **HWID (Hardware ID) Binding** - 1 license key = 1 machine
- **Auto-Suspension** - Automatic suspension if key used on multiple machines
- **IP Address Logging** - Track all login attempts with IP addresses
- **Login History** - Complete audit trail of all authentications
- **License Expiration** - Support for time-limited licenses (X days) or permanent
- **Comprehensive Admin API** - Full CRUD operations for license management

### ✅ Qt Client Application (`qt_client/`)
- **Modern Dark Theme UI** - Professional Qt-based interface
- **Key Saving** - Encrypted local storage of license keys
- **Auto-Login** - Automatic authentication on startup
- **HWID Detection** - Automatic machine fingerprinting
- **Cross-Platform** - Works on Windows, Linux, and macOS
- **User-Friendly** - Clean interface with status indicators

### 🚧 Admin Panel (In Progress - `qt_admin/`)
The admin panel foundation is created. To complete it, you'll need Qt Creator or can use the backend API directly via curl/Postman.

---

## 🚀 Quick Start

### 1. Install Requirements

**Backend:**
```bash
cd backend
pip install flask flask-sqlalchemy
```

**Qt Client (requires Qt 5.15+ or Qt 6):**
```bash
# Install Qt from https://www.qt.io/download
# Or use package manager:
# Ubuntu: sudo apt install qt6-base-dev qt6-tools-dev cmake
# macOS: brew install qt6
# Windows: Download Qt installer
```

### 2. Start the License Server

```bash
cd backend
python license_server.py
```

Server runs on `http://localhost:5000`

### 3. Create Your First License

**Using curl:**
```bash
curl -X POST http://localhost:5000/api/admin/licenses \
  -H "Content-Type: application/json" \
  -d '{"username": "john_doe", "days": 30}'
```

**Using Python:**
```python
import requests

response = requests.post('http://localhost:5000/api/admin/licenses', json={
    'username': 'john_doe',
    'days': 30  # 0 for permanent
})

print(response.json())
```

**Response:**
```json
{
  "success": true,
  "license_key": "abcd1234efgh5678...",
  "username": "john_doe",
  "expires_at": "2025-12-10T12:34:56",
  "days": 30,
  "note": "Save this license key securely - it cannot be retrieved again"
}
```

**IMPORTANT:** Save the `license_key`! It's only shown once.

### 4. Build and Run the Qt Client

```bash
cd qt_client
mkdir build
cd build
cmake ..
cmake --build .
./SecureLicenseClient  # or SecureLicenseClient.exe on Windows
```

### 5. Test Authentication

1. Run the client application
2. Paste your license key
3. Click "Login"
4. ✅ Success! Your machine is now bound to this license

Try logging in again from the same machine - it works!
Try the same key on a different machine - **auto-suspended** for security!

---

## 📊 Backend API Reference

### Authentication

#### `POST /api/validate`
Validate a license key with HWID binding.

**Request:**
```json
{
  "key": "your-license-key",
  "hwid": "machine-hardware-id"
}
```

**Response (Success):**
```json
{
  "success": true,
  "message": "Authentication successful",
  "username": "john_doe",
  "expires_at": "2025-12-10T12:34:56",
  "days_remaining": 29,
  "is_permanent": false
}
```

**Response (HWID Violation):**
```json
{
  "success": false,
  "message": "HWID mismatch detected. License has been automatically suspended for security.",
  "details": "This key is registered to another machine. Contact administrator."
}
```

### Admin Endpoints

#### `POST /api/admin/licenses` - Create License
```json
{
  "username": "user123",
  "days": 30  // 0 = permanent
}
```

#### `GET /api/admin/licenses` - List All Licenses
Returns all licenses with status, HWID, IP, expiration, etc.

#### `GET /api/admin/licenses/{id}` - Get License Details
Returns license info + complete login history with IPs and HWIDs.

#### `POST /api/admin/licenses/{id}/suspend` - Suspend License
```json
{
  "reason": "Violation of terms"
}
```

#### `POST /api/admin/licenses/{id}/unsuspend` - Unsuspend License

#### `POST /api/admin/licenses/{id}/reset-hwid` - Reset HWID Binding
Allows user to rebind to a new machine (useful for hardware upgrades).

#### `DELETE /api/admin/licenses/{id}` - Delete License

#### `POST /api/admin/licenses/{id}/extend` - Extend License
```json
{
  "days": 30
}
```

#### `GET /api/admin/stats` - Get Statistics
Returns:
- Total licenses
- Active licenses
- Suspended licenses
- Expired licenses
- Logins in last 24 hours

---

## 🔐 How It Works

### HWID (Hardware ID) Binding

When a user first logs in:
1. Client generates unique HWID from hardware components
2. Server receives license key + HWID
3. Server binds HWID to that license (first come, first served)
4. HWID is stored in database

On subsequent logins:
1. Client sends license key + HWID
2. Server checks if HWID matches stored value
3. ✅ Match = Login successful
4. ❌ Mismatch = **Auto-suspend** + Alert administrator

### HWID Generation (Qt Client)

The client creates a unique fingerprint from:
- Machine UUID
- MAC address (first network interface)
- System product type
- Kernel version
- Storage device identifier
- **All hashed with SHA-256**

This creates a consistent identifier per machine.

### Auto-Suspension

When HWID violation is detected:
```
License Status: SUSPENDED
Reason: "HWID violation detected. Registered: abc123..., Attempted: xyz789..."
```

Administrator must manually review and either:
- Reset HWID (legitimate hardware change)
- Keep suspended (attempted fraud)

### Login History Tracking

Every login attempt is logged:
- Timestamp
- IP address
- HWID used
- Success/failure
- Failure reason (if applicable)

View with: `GET /api/admin/licenses/{id}`

---

## 🖥️ Qt Client Features

### Key Saving
- Checkbox: "Remember my key (auto-login)"
- Keys are obfuscated (XOR + Base64) before storage
- Stored in OS-specific secure location:
  - Windows: Registry
  - Linux: ~/.config
  - macOS: ~/Library/Preferences

### Auto-Login
- If "Remember" is checked and key is saved
- Client auto-authenticates on startup
- 500ms delay to show window first

### HWID Detection
- Automatic - no user action required
- Consistent across reboots
- Changes if hardware changes

### Error Handling
- Clear error messages
- Auto-clears saved key on HWID violations
- Network error detection

---

## 🛠️ Building the Admin Panel (Complete It Yourself)

The admin panel starter is in `qt_admin/`. To complete it:

### Option 1: Use Qt Creator (Easiest)
1. Install Qt Creator
2. Open `qt_admin/CMakeLists.txt`
3. Create UI with Qt Designer
4. Implement these features:
   - License table (QTableWidget)
   - Create license dialog
   - License details dialog with history
   - Suspend/Delete/Extend buttons
   - Statistics dashboard

### Option 2: Use Backend API Directly
You can manage everything via HTTP requests:

```bash
# List all licenses
curl http://localhost:5000/api/admin/licenses

# Create license
curl -X POST http://localhost:5000/api/admin/licenses \
  -H "Content-Type: application/json" \
  -d '{"username": "test", "days": 7}'

# Suspend license
curl -X POST http://localhost:5000/api/admin/licenses/1/suspend \
  -H "Content-Type: application/json" \
  -d '{"reason": "Payment overdue"}'

# View login history
curl http://localhost:5000/api/admin/licenses/1
```

### Option 3: Web-Based Admin Panel
Create a simple Flask web UI:

```python
from flask import render_template

@app.route('/admin/dashboard')
def admin_dashboard():
    licenses = LicenseKey.query.all()
    return render_template('dashboard.html', licenses=licenses)
```

---

## 📈 Example Workflows

### Workflow 1: New Customer
```bash
# 1. Create 30-day license
curl -X POST http://localhost:5000/api/admin/licenses \
  -H "Content-Type: application/json" \
  -d '{"username": "customer1", "days": 30}'

# 2. Send license key to customer
# 3. Customer enters key in Qt client
# 4. HWID automatically bound
# 5. Customer can use for 30 days
```

### Workflow 2: License Expires
```bash
# Check expiration
curl http://localhost:5000/api/admin/licenses/1

# Extend by 30 days
curl -X POST http://localhost:5000/api/admin/licenses/1/extend \
  -H "Content-Type: application/json" \
  -d '{"days": 30}'
```

### Workflow 3: Customer Changes Hardware
```bash
# Customer reports can't login after hardware upgrade
# Check license
curl http://localhost:5000/api/admin/licenses/1

# See it's suspended due to HWID mismatch
# Reset HWID to allow rebinding
curl -X POST http://localhost:5000/api/admin/licenses/1/reset-hwid

# Customer can now login and rebind to new hardware
```

### Workflow 4: Fraud Detection
```bash
# Check login history
curl http://localhost:5000/api/admin/licenses/1

# See multiple different HWIDs
# License is auto-suspended
# Review and decide:
# - Legitimate? Reset HWID
# - Fraud? Keep suspended or delete
```

---

## 🔒 Security Best Practices

### Production Deployment

1. **Use HTTPS**: Never use HTTP in production
   ```python
   # Use SSL context
   app.run(ssl_context='adhoc')  # Development
   # Or use nginx/Apache as reverse proxy
   ```

2. **Secure the Admin API**: Add authentication
   ```python
   @app.route('/api/admin/licenses', methods=['POST'])
   @require_admin_auth
   def create_license():
       # ...
   ```

3. **Rate Limiting**: Prevent brute force
   ```python
   from flask_limiter import Limiter

   limiter = Limiter(app, key_func=get_remote_address)

   @app.route('/api/validate')
   @limiter.limit("10 per minute")
   def validate_license():
       # ...
   ```

4. **Database Encryption**: Encrypt sensitive data

5. **Logging**: Monitor suspicious activity
   ```python
   import logging

   logging.warning(f"HWID violation: {license_id} from {ip_address}")
   ```

6. **Backup**: Regular database backups
   ```bash
   cp license.db license_backup_$(date +%Y%m%d).db
   ```

---

## 📦 What's Included

```
basic_login/
├── backend/
│   ├── license_server.py       ← NEW: Professional license server
│   ├── oauth_app.py             ← OAuth 2.0 server
│   └── app.py                   ← Original simple key auth
│
├── qt_client/                   ← NEW: Modern Qt client
│   ├── main.cpp
│   ├── mainwindow.h/cpp/ui
│   └── CMakeLists.txt
│
├── qt_admin/                    ← NEW: Admin panel (starter)
│   └── main.cpp
│
└── client/                      ← Old Win32 client (deprecated)
    └── main.cpp
```

---

## 🎨 Qt Client vs Old Win32 Client

| Feature | Old Win32 | New Qt Client |
|---------|-----------|---------------|
| Cross-platform | ❌ Windows only | ✅ Win/Linux/macOS |
| UI Framework | Win32 API | Qt (modern) |
| Key Saving | ❌ No | ✅ Yes |
| Auto-Login | ❌ No | ✅ Yes |
| HWID Binding | ❌ No | ✅ Yes |
| Modern Look | ⚠️ Basic | ✅ Professional |
| Easy to Modify | ❌ Hard | ✅ Qt Designer |

**Recommendation:** Use the Qt client for production.

---

## 🐛 Troubleshooting

**Qt Client won't build:**
- Install Qt: `https://www.qt.io/download`
- Set CMAKE_PREFIX_PATH: `export CMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64`

**License auto-suspended:**
- Different HWID detected
- Use `/reset-hwid` endpoint to allow rebinding

**Can't connect to server:**
- Ensure `license_server.py` is running
- Check firewall settings
- Verify URL in client code

**Key not saving:**
- Check file permissions
- Verify QSettings path

---

## 🎯 Next Steps

1. **Complete Admin Panel** - Use Qt Creator to build full UI
2. **Add Payment Integration** - Stripe, PayPal, etc.
3. **Email Notifications** - Expiration reminders
4. **Web Dashboard** - Browser-based admin interface
5. **License Analytics** - Usage statistics, charts
6. **Multi-Tier Licenses** - Basic, Pro, Enterprise
7. **Feature Flags** - Different features per license tier

---

## 📞 Support

- Backend API: `http://localhost:5000/health`
- Documentation: This file
- Source: Check git history for implementation details

**You now have a professional-grade license management system! 🎉**
