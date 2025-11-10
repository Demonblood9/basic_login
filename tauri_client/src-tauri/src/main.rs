// Prevents additional console window on Windows in release
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use serde::{Deserialize, Serialize};
use std::collections::hash_map::DefaultHasher;
use std::hash::{Hash, Hasher};
use tauri::Manager;

#[derive(Debug, Serialize, Deserialize)]
struct LoginRequest {
    key: String,
    hwid: String,
}

#[derive(Debug, Serialize, Deserialize)]
struct LoginResponse {
    success: bool,
    message: Option<String>,
    username: Option<String>,
    expires_at: Option<String>,
    time_remaining: Option<String>,
    is_permanent: Option<bool>,
}

// Get Hardware ID (HWID)
#[tauri::command]
async fn get_hwid() -> Result<String, String> {
    let mut hasher = DefaultHasher::new();

    // Get machine ID
    if let Ok(machine_id) = machine_uid::get() {
        machine_id.hash(&mut hasher);
    }

    // Get MAC address (Windows-specific)
    #[cfg(target_os = "windows")]
    {
        if let Ok(mac) = get_windows_mac_address() {
            mac.hash(&mut hasher);
        }
    }

    // Get system info
    let os_info = std::env::consts::OS;
    os_info.hash(&mut hasher);

    Ok(format!("{:x}", hasher.finish()))
}

#[cfg(target_os = "windows")]
fn get_windows_mac_address() -> Result<String, String> {
    use windows::Win32::NetworkManagement::IpHelper::{
        GetAdaptersAddresses, GAA_FLAG_INCLUDE_PREFIX, IP_ADAPTER_ADDRESSES_LH,
    };
    use windows::Win32::Foundation::ERROR_SUCCESS;
    use std::mem;

    unsafe {
        let mut size: u32 = 0;

        // First call to get size
        GetAdaptersAddresses(
            0, // AF_UNSPEC
            GAA_FLAG_INCLUDE_PREFIX,
            None,
            None,
            &mut size,
        );

        let mut buffer = vec![0u8; size as usize];
        let adapter_addresses = buffer.as_mut_ptr() as *mut IP_ADAPTER_ADDRESSES_LH;

        let result = GetAdaptersAddresses(
            0,
            GAA_FLAG_INCLUDE_PREFIX,
            None,
            Some(adapter_addresses),
            &mut size,
        );

        if result == ERROR_SUCCESS {
            let mut current = adapter_addresses;
            while !current.is_null() {
                let adapter = &*current;
                if adapter.PhysicalAddressLength > 0 {
                    let mac: Vec<String> = adapter.PhysicalAddress[..adapter.PhysicalAddressLength as usize]
                        .iter()
                        .map(|b| format!("{:02X}", b))
                        .collect();
                    return Ok(mac.join(":"));
                }
                current = adapter.Next;
            }
        }
    }

    Err("No MAC address found".to_string())
}

// Validate license key
#[tauri::command]
async fn validate_license(key: String, hwid: String) -> Result<LoginResponse, String> {
    let client = reqwest::Client::new();

    let request = LoginRequest {
        key: key.clone(),
        hwid: hwid.clone(),
    };

    match client
        .post("http://localhost:5000/api/validate")
        .json(&request)
        .send()
        .await
    {
        Ok(response) => {
            match response.json::<LoginResponse>().await {
                Ok(data) => Ok(data),
                Err(e) => Err(format!("Failed to parse response: {}", e)),
            }
        }
        Err(e) => Err(format!("Connection error: {}", e)),
    }
}

// Save license key to system keyring
#[tauri::command]
fn save_license_key(key: String) -> Result<(), String> {
    let entry = keyring::Entry::new("SecureLicense", "license_key")
        .map_err(|e| format!("Keyring error: {}", e))?;

    entry
        .set_password(&key)
        .map_err(|e| format!("Failed to save key: {}", e))
}

// Load license key from system keyring
#[tauri::command]
fn load_license_key() -> Result<String, String> {
    let entry = keyring::Entry::new("SecureLicense", "license_key")
        .map_err(|e| format!("Keyring error: {}", e))?;

    entry
        .get_password()
        .map_err(|e| format!("No saved key: {}", e))
}

// Delete license key from system keyring
#[tauri::command]
fn delete_license_key() -> Result<(), String> {
    let entry = keyring::Entry::new("SecureLicense", "license_key")
        .map_err(|e| format!("Keyring error: {}", e))?;

    entry
        .delete_password()
        .map_err(|e| format!("Failed to delete key: {}", e))
}

// Check if license key exists
#[tauri::command]
fn has_saved_key() -> bool {
    if let Ok(entry) = keyring::Entry::new("SecureLicense", "license_key") {
        entry.get_password().is_ok()
    } else {
        false
    }
}

fn main() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![
            get_hwid,
            validate_license,
            save_license_key,
            load_license_key,
            delete_license_key,
            has_saved_key,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
