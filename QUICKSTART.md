# Quick Start Guide

Get up and running with the Secure Key-Based Login System in 5 minutes!

## Step 1: Start the Backend (2 minutes)

```bash
# Navigate to backend directory
cd backend

# Install dependencies
pip install -r requirements.txt

# Start the server
python app.py
```

You should see:
```
Database initialized successfully
 * Running on http://0.0.0.0:5000
```

## Step 2: Create Your First User (30 seconds)

Open a new terminal/command prompt:

```bash
cd backend
python manage_users.py create john_doe
```

You'll receive an access key like:
```
✓ User created successfully!
Username: john_doe
Access Key: Xj7K9mPqRt2WvYz5BnHdFg8LcNx4QwEaS1-_0oIuJp3M6
```

**IMPORTANT:** Copy this key somewhere safe! You'll need it to login.

## Step 3: Build the Client (2 minutes)

### On Windows:

```cmd
cd client
build.bat
```

The script will automatically detect your compiler (MinGW or MSVC) and build the client.

## Step 4: Login! (30 seconds)

1. Run `SecureLoginClient.exe`
2. Paste your access key
3. Click "Login"
4. Success! 🎉

## Verify Everything Works

Test your key from command line:

```bash
cd backend
python manage_users.py test YOUR-ACCESS-KEY-HERE
```

You should see:
```
✓ Key is valid!
Username: john_doe
Last Login: 2025-11-10T12:34:56
```

## What's Next?

- Create more users with different access keys
- Try deactivating/activating users
- Integrate the authentication into your application
- Read the full [README.md](README.md) for advanced features

## Common Issues

**Backend won't start?**
- Make sure port 5000 is not in use
- Check that Python 3.7+ is installed: `python --version`

**Client won't build?**
- Install MinGW-w64 or Visual Studio
- Make sure compiler is in your PATH

**Client can't connect?**
- Ensure backend is running on localhost:5000
- Check Windows Firewall settings

**Need help?** Check the [README.md](README.md) or open an issue!
