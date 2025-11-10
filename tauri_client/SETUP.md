# Quick Setup Guide

## Step 1: Install Prerequisites

### Windows Users:

1. **Install Rust** (PowerShell):
```powershell
winget install Rustlang.Rust.GNU
```
Or download from: https://rustup.rs/

2. **Install Node.js**:
Download from: https://nodejs.org/ (LTS version)

3. **Install Visual Studio C++ Build Tools**:
Download from: https://visualstudio.microsoft.com/visual-cpp-build-tools/
- Select "Desktop development with C++"
- Install

4. **Install WebView2** (usually pre-installed on Windows 11):
https://developer.microsoft.com/en-us/microsoft-edge/webview2/

## Step 2: Install Dependencies

Open terminal in `tauri_client` directory:

```bash
npm install
```

## Step 3: Run Development Mode

```bash
npm run tauri dev
```

The app will launch automatically!

## Step 4: Build Production App

```bash
npm run tauri build
```

Find your executable in:
- Windows: `src-tauri/target/release/secure-license-client.exe`
- Installer: `src-tauri/target/release/bundle/msi/*.msi`

## Troubleshooting

### Error: "WebView2 not found"
Install WebView2 Runtime from the link in Step 1

### Error: "cargo command not found"
Restart your terminal after installing Rust

### Error: "MSBuild not found"
Install Visual Studio C++ Build Tools (Step 1.3)

### Error: "Failed to compile"
Make sure all prerequisites are installed and try:
```bash
cd src-tauri
cargo clean
cd ..
npm run tauri build
```

## Next Steps

1. ✅ App built successfully!
2. Test the login with your Flask server running
3. Customize the UI in `src/styles.css`
4. Add your app icons to `src-tauri/icons/`
5. Configure auto-updates in `src-tauri/tauri.conf.json`

## Comparison with Qt

| Feature | Qt Client | Tauri Client |
|---------|-----------|--------------|
| File Size | ~50MB | ~5-10MB |
| Memory | ~150MB | ~50-100MB |
| UI Framework | Qt Widgets | Modern Web (HTML/CSS/JS) |
| Theme | Basic | Glassmorphism, animations |
| Updates | Custom implementation | Built-in updater |
| Startup | ~2-3s | <1s |
| Cross-platform | Yes | Yes |
| Modern Look | ❌ | ✅ |

Enjoy your modern, lightweight desktop app! 🚀
