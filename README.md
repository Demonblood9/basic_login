# Secure Key-Based Login System

A secure authentication system using unique access keys instead of traditional username/password. The system consists of a Python Flask backend and a C++ Windows GUI client.

## Features

- **Key-Based Authentication**: Each user receives a unique, cryptographically secure access key
- **Secure Storage**: Keys are hashed using SHA-256 before storage
- **REST API Backend**: Flask-based API for key validation and user management
- **Native Windows GUI**: Lightweight C++ client using Win32 API
- **User Management**: CLI tools for creating, listing, and managing users
- **Activity Tracking**: Tracks user login history and account status

## Project Structure

```
basic_login/
├── backend/                 # Python Flask backend
│   ├── app.py              # Main Flask application
│   ├── manage_users.py     # User management CLI tool
│   └── requirements.txt    # Python dependencies
│
├── client/                 # C++ Windows GUI client
│   ├── main.cpp           # Main application code
│   ├── Makefile           # Build file for MinGW/Make
│   ├── CMakeLists.txt     # CMake build configuration
│   └── build.bat          # Windows build script
│
└── README.md              # This file
```

## Backend Setup (Python Flask)

### Prerequisites

- Python 3.7 or higher
- pip (Python package manager)

### Installation

1. Navigate to the backend directory:
   ```bash
   cd backend
   ```

2. Install required packages:
   ```bash
   pip install -r requirements.txt
   ```

3. Start the Flask server:
   ```bash
   python app.py
   ```

   The server will start on `http://localhost:5000`

### User Management

Use the `manage_users.py` script to manage users and access keys:

#### Create a new user:
```bash
python manage_users.py create john_doe
```

This will generate a unique access key. **Save this key securely** - it cannot be retrieved again!

Example output:
```
✓ User created successfully!
Username: john_doe
Access Key: abcdef123456...

Save this key securely - it cannot be retrieved again
```

#### List all users:
```bash
python manage_users.py list
```

#### Deactivate a user:
```bash
python manage_users.py deactivate 1
```

#### Activate a user:
```bash
python manage_users.py activate 1
```

#### Test an access key:
```bash
python manage_users.py test your-access-key-here
```

## Client Setup (C++ Windows GUI)

### Prerequisites

Choose one of the following compilers:
- **MinGW-w64**: Download from [mingw-w64.org](https://www.mingw-w64.org/)
- **Visual Studio**: Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/)

### Building the Client

#### Option 1: Using the build script (Easiest)
```cmd
cd client
build.bat
```

#### Option 2: Using Make (with MinGW)
```cmd
cd client
mingw32-make
```

#### Option 3: Using CMake
```cmd
cd client
mkdir build
cd build
cmake ..
cmake --build .
```

#### Option 4: Manual compilation with MinGW
```cmd
cd client
g++ -std=c++11 -municode -mwindows -O2 -Wall main.cpp -o SecureLoginClient.exe -lwinhttp -lcomctl32 -lgdi32
```

#### Option 5: Manual compilation with MSVC
```cmd
cd client
cl.exe /EHsc /Fe:SecureLoginClient.exe main.cpp /link winhttp.lib comctl32.lib gdi32.lib user32.lib
```

### Running the Client

1. Make sure the Flask backend is running on `localhost:5000`

2. Run the compiled executable:
   ```cmd
   SecureLoginClient.exe
   ```

3. Enter your access key in the GUI and click "Login"

## API Endpoints

### Public Endpoints

#### `POST /api/validate`
Validate an access key.

**Request:**
```json
{
  "key": "your-access-key-here"
}
```

**Response (Success - 200):**
```json
{
  "success": true,
  "message": "Authentication successful",
  "username": "john_doe",
  "last_login": "2025-11-10T12:34:56"
}
```

**Response (Failure - 401):**
```json
{
  "success": false,
  "message": "Invalid or inactive key"
}
```

#### `GET /api/health`
Health check endpoint.

**Response:**
```json
{
  "status": "healthy",
  "timestamp": "2025-11-10T12:34:56"
}
```

### Admin Endpoints

#### `POST /api/admin/create_user`
Create a new user and generate an access key.

**Request:**
```json
{
  "username": "new_user"
}
```

#### `GET /api/admin/list_users`
List all users in the system.

#### `POST /api/admin/deactivate_user/<user_id>`
Deactivate a user account.

#### `POST /api/admin/activate_user/<user_id>`
Activate a user account.

## Security Features

### Key Generation
- Uses Python's `secrets` module for cryptographically secure random key generation
- Keys are 32 bytes (256 bits) encoded in URL-safe base64 format
- Each key is unique and unpredictable

### Key Storage
- Keys are hashed using SHA-256 before storage in the database
- Original keys are never stored (except during initial generation for user retrieval)
- Hash comparison prevents key exposure even if database is compromised

### Communication
- Client uses WinHTTP for reliable HTTP communication
- JSON-based API for structured data exchange
- Input validation on both client and server

### Best Practices
1. **Never share access keys** - Each user should have their own key
2. **Secure key storage** - Store keys securely (password managers, encrypted files)
3. **Regular key rotation** - Deactivate and regenerate keys periodically
4. **Monitor login activity** - Check last_login timestamps for suspicious activity
5. **Use HTTPS in production** - Configure SSL/TLS for the Flask backend

## Configuration

### Backend Configuration

Edit `backend/app.py` to customize:

```python
# Database location
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///keys.db'

# Server settings (in main block)
app.run(host='0.0.0.0', port=5000, debug=True)
```

### Client Configuration

Edit `client/main.cpp` to customize:

```cpp
// Server configuration
const wchar_t* SERVER_HOST = L"localhost";
const int SERVER_PORT = 5000;
const wchar_t* VALIDATE_PATH = L"/api/validate";
```

## Production Deployment

### Backend (Flask)

For production, use a proper WSGI server like Gunicorn:

```bash
pip install gunicorn
gunicorn -w 4 -b 0.0.0.0:5000 app:app
```

### Security Recommendations

1. **Use HTTPS**: Configure SSL/TLS certificates
2. **Add authentication to admin endpoints**: Implement API key or OAuth
3. **Rate limiting**: Implement rate limiting to prevent brute-force attacks
4. **Database backups**: Regular backups of the SQLite database
5. **Environment variables**: Store sensitive config in environment variables
6. **Firewall rules**: Restrict backend access to authorized IPs
7. **Logging**: Implement comprehensive logging for security auditing

## Troubleshooting

### Backend Issues

**Problem:** `ModuleNotFoundError` when running Flask
- **Solution:** Install requirements: `pip install -r requirements.txt`

**Problem:** Port 5000 already in use
- **Solution:** Change port in `app.py` or kill the process using port 5000

**Problem:** Database locked error
- **Solution:** Ensure only one instance of the Flask app is running

### Client Issues

**Problem:** Cannot connect to server
- **Solution:** Ensure Flask backend is running and accessible on localhost:5000

**Problem:** Build errors on Windows
- **Solution:** Ensure you have MinGW or Visual Studio installed and in PATH

**Problem:** "Invalid or inactive key" message
- **Solution:** Verify the key is correct and the user is active (check with `manage_users.py list`)

## Testing

### Test the complete system:

1. Start the backend:
   ```bash
   cd backend
   python app.py
   ```

2. Create a test user:
   ```bash
   python manage_users.py create testuser
   ```

3. Copy the generated access key

4. Run the client and paste the key

5. Click "Login" - you should see a success message!

## License

This project is provided as-is for educational and commercial use.

## Contributing

Feel free to submit issues, fork the repository, and create pull requests for any improvements.

## Future Enhancements

- [ ] HTTPS support in client
- [ ] Key expiration and rotation
- [ ] Multi-factor authentication
- [ ] Desktop notifications
- [ ] Cross-platform client (Linux, macOS)
- [ ] Web-based client interface
- [ ] Role-based access control
- [ ] Audit logging system
- [ ] Password recovery mechanism (for key replacement)

## Support

For issues or questions, please open an issue on the project repository.
