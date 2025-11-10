# Web Admin Panel - Quick Start Guide

## 🚀 Getting Started in 4 Steps

### 1. Create Your Admin Account (First Time Only)

**On Windows:**
```cmd
cd backend
create_admin.bat
```

**On Linux/Mac:**
```bash
cd backend
python create_admin.py
```

Follow the prompts to create your admin username and password.

**Important:** Remember your credentials - you'll need them to log in!

### 2. Start the Server
```bash
cd backend
python license_server.py
```

Or use the startup script:
```bash
cd backend
./start_server.sh
```

### 3. Log In to the Admin Panel

Open your web browser and navigate to:
```
http://localhost:5000/admin/login
```

Enter the username and password you created in Step 1.

### 4. Start Managing Licenses!
- Click "Create New License" to generate license keys
- View all licenses in the "All Licenses" page
- Click on any license to see detailed information and login history

---

## 📋 Admin Panel Pages

### Dashboard (`/admin/dashboard`)
Your control center with:
- Total licenses count
- Active licenses count
- Suspended licenses count
- Expired licenses count
- Recent login activity (24 hours)
- Quick access to recent licenses

### All Licenses (`/admin/licenses`)
Complete list of all license keys with:
- Status badges (Active/Suspended/Expired)
- HWID binding status
- IP addresses
- Expiration dates
- Quick view buttons

### Create License (`/admin/licenses/create`)
Generate new license keys:
- Enter username
- Choose duration:
  - Preset options (7, 14, 30, 60, 90, 180, 365 days)
  - Permanent (never expires)
  - Custom (any number of days)
- License key shown only once after creation!

### License Details (`/admin/licenses/<id>`)
Detailed view with:
- Complete license information
- HWID and IP address
- Management actions:
  - **Suspend/Unsuspend** - Block or restore access
  - **Reset HWID** - Allow rebinding to new machine
  - **Extend License** - Add more days
  - **Delete License** - Permanent removal
- Complete login history with timestamps, IPs, and HWIDs

---

## 🔐 Security & Authentication

### Admin Login System
The admin panel is now protected with secure authentication:

✅ **Session-based authentication** - Secure login required for all admin pages
✅ **Password hashing** - Passwords stored with SHA-256 hashing
✅ **Auto-redirect** - Unauthorized users redirected to login page
✅ **Flash messages** - Clear feedback for login attempts
✅ **User display** - Shows logged-in username in sidebar
✅ **Logout functionality** - Secure session termination

### Managing Admin Users

**Create Additional Admins:**
```bash
cd backend
python create_admin.py
```

**Protected Routes:**
All admin panel routes require authentication:
- `/admin/dashboard`
- `/admin/licenses`
- `/admin/licenses/create`
- `/admin/licenses/<id>` (details, suspend, delete, etc.)

**Public Routes:**
- `/admin/login` - Login page
- `/api/validate` - Client license validation (no auth required)
- `/health` - Server health check

### Best Practices

1. **Use Strong Passwords** - Minimum 6 characters (longer recommended)
2. **Don't Share Credentials** - Each admin should have their own account
3. **Log Out When Done** - Click logout button in sidebar
4. **Monitor Login Activity** - Check license login history regularly
5. **Use HTTPS in Production** - Never use HTTP for production deployments

---

## 🔑 Common Tasks

### Creating a License
1. Go to `/admin/licenses/create`
2. Enter username (e.g., "john_doe")
3. Select duration (e.g., 30 days)
4. Click "Generate License Key"
5. **IMPORTANT**: Copy the license key immediately!
6. Send the key to the user

### Handling HWID Violations
When a user tries to use their key on a different machine:
1. License automatically gets suspended
2. View license details to see the violation
3. Decide:
   - **Legitimate** (hardware upgrade): Click "Reset HWID"
   - **Fraud** (key sharing): Keep suspended or delete

### Extending Expired Licenses
1. Go to license details page
2. Click "Extend License"
3. Choose number of days to add
4. Confirm

### Viewing Login History
1. Go to license details page
2. Scroll to "Login History" section
3. See all login attempts with:
   - Timestamp
   - IP address
   - HWID used
   - Success/failure status
   - Failure reasons

---

## 🎨 Features

### Security
✅ HWID binding (1 license = 1 machine)
✅ Automatic suspension on HWID violations
✅ IP address tracking
✅ Complete login audit trail
✅ SHA-256 key hashing

### User Management
✅ Create licenses with custom durations
✅ Suspend/unsuspend licenses
✅ Reset HWID bindings
✅ Extend license durations
✅ Delete licenses permanently

### Interface
✅ Modern Bootstrap 5 design
✅ Responsive layout (works on mobile)
✅ Real-time statistics
✅ Flash messages for actions
✅ Confirmation modals for destructive actions

---

## 📊 Statistics Explained

- **Total Licenses**: All licenses ever created
- **Active**: Currently valid and not suspended
- **Suspended**: Blocked (usually due to HWID violations)
- **Expired**: Past their expiration date
- **Logins (24h)**: Successful login attempts in last 24 hours

---

## 🔧 API Access

You can also manage licenses via the REST API:

### List all licenses
```bash
curl http://localhost:5000/api/admin/licenses
```

### Create a license
```bash
curl -X POST http://localhost:5000/api/admin/licenses \
  -H "Content-Type: application/json" \
  -d '{"username": "test_user", "days": 30}'
```

### Get license details
```bash
curl http://localhost:5000/api/admin/licenses/1
```

### Suspend a license
```bash
curl -X POST http://localhost:5000/api/admin/licenses/1/suspend \
  -H "Content-Type: application/json" \
  -d '{"reason": "Payment overdue"}'
```

### Unsuspend a license
```bash
curl -X POST http://localhost:5000/api/admin/licenses/1/unsuspend
```

### Reset HWID
```bash
curl -X POST http://localhost:5000/api/admin/licenses/1/reset-hwid
```

### Extend license
```bash
curl -X POST http://localhost:5000/api/admin/licenses/1/extend \
  -H "Content-Type: application/json" \
  -d '{"days": 30}'
```

### Delete license
```bash
curl -X DELETE http://localhost:5000/api/admin/licenses/1
```

---

## 🐛 Troubleshooting

### Server won't start
- Check if port 5000 is already in use
- Ensure Flask and Flask-SQLAlchemy are installed: `pip install flask flask-sqlalchemy`

### Can't access admin panel
- Verify server is running
- Check URL: `http://localhost:5000/admin/dashboard` (not 127.0.0.1)
- Check firewall settings

### License creation fails
- Ensure username is unique
- Check server console for error messages

### Templates not loading
- Verify `backend/templates/` directory exists
- Ensure all template files are present:
  - base.html
  - dashboard.html
  - licenses.html
  - license_details.html
  - license_created.html
  - create_license.html

---

## 📚 Additional Documentation

For complete documentation, see:
- **LICENSE_SYSTEM_GUIDE.md** - Comprehensive system guide
- **OAUTH2_GUIDE.md** - OAuth 2.0 implementation details
- **README.md** - General project overview

---

## ✨ Tips

1. **License keys are shown only once** - Make sure to copy them immediately after creation!

2. **HWID reset is for legitimate cases** - Use it when users upgrade hardware, not for fraud

3. **Check login history regularly** - Spot unusual activity patterns

4. **Use permanent licenses sparingly** - Time-limited licenses give you more control

5. **Dashboard statistics update in real-time** - Refresh to see latest numbers

---

**You're all set! Your professional license management system is ready to use! 🎉**
