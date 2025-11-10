# Secure License Client - Tauri Desktop App

A modern, lightweight desktop application for secure license authentication built with Tauri (Rust + Web Technologies).

## Features

- 🎨 Beautiful modern dark UI with glassmorphism effects
- 🔐 Secure HWID-based authentication
- 💾 Secure key storage using system keyring
- 🚀 Lightweight (~5-10MB executable)
- 🔄 Auto-update support
- ⚡ Fast and responsive
- 🎯 Cross-platform (Windows, macOS, Linux)

## Prerequisites

### Install Rust
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

### Install Node.js
Download from https://nodejs.org/ (v16 or higher)

### Windows Additional Requirements
Install Microsoft Visual Studio C++ Build Tools:
https://visualstudio.microsoft.com/visual-cpp-build-tools/

Install WebView2:
https://developer.microsoft.com/en-us/microsoft-edge/webview2/

## Installation

1. Navigate to the tauri_client directory:
```bash
cd tauri_client
```

2. Install npm dependencies:
```bash
npm install
```

## Development

Run the app in development mode:
```bash
npm run tauri dev
```

## Building

Build the production executable:
```bash
npm run tauri build
```

The built executable will be in `src-tauri/target/release/`

For Windows, you'll also find an installer in `src-tauri/target/release/bundle/msi/` or `src-tauri/target/release/bundle/nsis/`

## Building for Different Platforms

### Windows
```bash
npm run tauri build
```
Output: `.exe` and `.msi` installer

### macOS
```bash
npm run tauri build -- --target universal-apple-darwin
```
Output: `.app` bundle and `.dmg`

### Linux
```bash
npm run tauri build
```
Output: `.AppImage`, `.deb`, and `.rpm`

## Configuration

Edit `src-tauri/tauri.conf.json` to customize:
- Window size and appearance
- App name and identifier
- Update server URL
- Icons and assets

## Icons

Place your app icons in `src-tauri/icons/`:
- `32x32.png`
- `128x128.png`
- `128x128@2x.png`
- `icon.icns` (macOS)
- `icon.ico` (Windows)

You can generate icons using: https://github.com/tauri-apps/tauricon

## API Configuration

The app connects to the Flask backend at `http://localhost:5000/api/validate`

To change this, edit `src-tauri/src/main.rs`:
```rust
.post("http://localhost:5000/api/validate")
```

## Features Implemented

### Rust Backend (`src-tauri/src/main.rs`)
- ✅ HWID generation (machine ID + MAC address)
- ✅ License validation API calls
- ✅ Secure keyring storage for license keys
- ✅ Windows-specific system integration

### Frontend (`src/`)
- ✅ Modern responsive UI with animations
- ✅ Form validation and error handling
- ✅ Success/error/suspended status displays
- ✅ Remember me functionality
- ✅ Auto-login with saved credentials
- ✅ Loading states and transitions

## Architecture

```
tauri_client/
├── src/                      # Frontend (HTML/CSS/JS)
│   ├── index.html           # Main UI
│   ├── styles.css           # Modern styling
│   └── main.js              # Application logic
├── src-tauri/               # Rust backend
│   ├── src/
│   │   └── main.rs          # Tauri commands
│   ├── Cargo.toml           # Rust dependencies
│   ├── tauri.conf.json      # Tauri configuration
│   └── build.rs             # Build script
└── package.json             # Node dependencies
```

## Security Features

- License keys stored in system keyring (not plain text)
- HWID-based device binding
- Secure HTTPS API calls (configure in production)
- No sensitive data in localStorage
- Native security boundaries

## Troubleshooting

### "WebView2 not found" (Windows)
Install WebView2 Runtime: https://developer.microsoft.com/en-us/microsoft-edge/webview2/

### Build fails on Windows
Make sure Visual Studio C++ Build Tools are installed

### Build fails on macOS
Install Xcode Command Line Tools:
```bash
xcode-select --install
```

### "Failed to get HWID"
Ensure the app has proper permissions to access system information

## Updating

The app includes auto-update support. Configure the update server in `tauri.conf.json`:
```json
"updater": {
  "active": true,
  "endpoints": [
    "https://your-server.com/api/tauri-update/{{target}}/{{current_version}}"
  ]
}
```

## Distribution

### Windows
- Distribute the `.exe` for standalone
- Use `.msi` installer for enterprise deployment
- Sign your executable for trusted installation

### macOS
- Distribute `.dmg` for easy installation
- Sign and notarize for Gatekeeper
- Consider Mac App Store distribution

### Linux
- `.AppImage` works on most distributions
- `.deb` for Debian/Ubuntu
- `.rpm` for Fedora/RedHat

## Performance

- App size: ~5-10MB (vs ~150MB for Electron)
- Memory usage: ~50-100MB
- Startup time: < 1 second
- Native performance with Rust backend

## License

[Your License Here]
