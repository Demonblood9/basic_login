const { invoke } = window.__TAURI__.tauri;

// DOM Elements
const loginForm = document.getElementById('loginForm');
const licenseKeyInput = document.getElementById('licenseKey');
const rememberMeCheckbox = document.getElementById('rememberMe');
const loginBtn = document.getElementById('loginBtn');
const clearBtn = document.getElementById('clearBtn');
const loadingOverlay = document.getElementById('loadingOverlay');
const statusDisplay = document.getElementById('statusDisplay');
const successDisplay = document.getElementById('successDisplay');
const statusIcon = document.getElementById('statusIcon');
const statusTitle = document.getElementById('statusTitle');
const statusMessage = document.getElementById('statusMessage');
const successDetails = document.getElementById('successDetails');

let currentHwid = '';

// Initialize
async function initialize() {
    try {
        // Get HWID
        currentHwid = await invoke('get_hwid');
        console.log('HWID:', currentHwid);

        // Check if key is saved
        const hasSaved = await invoke('has_saved_key');

        if (hasSaved) {
            rememberMeCheckbox.checked = true;
            try {
                const savedKey = await invoke('load_license_key');
                licenseKeyInput.value = savedKey;
                // Auto-login after a short delay
                setTimeout(() => handleLogin(), 500);
            } catch (error) {
                console.error('Failed to load saved key:', error);
            }
        }
    } catch (error) {
        console.error('Initialization error:', error);
        showError('Initialization Error', 'Failed to initialize application');
    }
}

// Handle Login
async function handleLogin() {
    const licenseKey = licenseKeyInput.value.trim();

    if (!licenseKey) {
        showError('Invalid Input', 'Please enter your license key');
        return;
    }

    // Clear previous status
    hideStatus();
    hideSuccess();

    // Show loading
    showLoading();
    disableForm();

    try {
        // Validate license
        const response = await invoke('validate_license', {
            key: licenseKey,
            hwid: currentHwid
        });

        hideLoading();

        if (response.success) {
            // Save key if remember me is checked
            if (rememberMeCheckbox.checked) {
                try {
                    await invoke('save_license_key', { key: licenseKey });
                } catch (error) {
                    console.error('Failed to save key:', error);
                }
            } else {
                // Delete saved key if remember me is unchecked
                try {
                    await invoke('delete_license_key');
                } catch (error) {
                    // Ignore error if key doesn't exist
                }
            }

            showSuccess(response);
        } else {
            // Check if suspended
            const message = response.message || 'Authentication failed';

            if (message.toLowerCase().includes('suspended')) {
                showSuspended(message);

                // Clear saved key
                try {
                    await invoke('delete_license_key');
                    rememberMeCheckbox.checked = false;
                } catch (error) {
                    console.error('Failed to delete key:', error);
                }
            } else {
                showError('Authentication Failed', message);

                // Clear saved key if HWID error
                if (message.toLowerCase().includes('hwid')) {
                    try {
                        await invoke('delete_license_key');
                        rememberMeCheckbox.checked = false;
                    } catch (error) {
                        console.error('Failed to delete key:', error);
                    }
                }
            }
        }
    } catch (error) {
        hideLoading();
        showError('Connection Error', error.toString());
    } finally {
        enableForm();
    }
}

// Handle Clear
async function handleClear() {
    licenseKeyInput.value = '';
    hideStatus();
    hideSuccess();

    if (rememberMeCheckbox.checked) {
        const confirmed = confirm('Do you want to remove the saved license key?');
        if (confirmed) {
            try {
                await invoke('delete_license_key');
                rememberMeCheckbox.checked = false;
            } catch (error) {
                console.error('Failed to delete key:', error);
            }
        }
    }
}

// Show Loading
function showLoading() {
    loadingOverlay.classList.remove('hidden');
}

function hideLoading() {
    loadingOverlay.classList.add('hidden');
}

// Show Error
function showError(title, message) {
    statusDisplay.classList.remove('hidden', 'suspended');
    statusDisplay.classList.add('error');
    statusIcon.textContent = '✗';
    statusTitle.textContent = title;
    statusMessage.textContent = message;
}

// Show Suspended
function showSuspended(message) {
    statusDisplay.classList.remove('hidden', 'error');
    statusDisplay.classList.add('suspended');
    statusIcon.textContent = '⚠';
    statusTitle.textContent = 'License Suspended';
    statusMessage.textContent = message + '\n\nPlease contact your administrator for assistance.';
}

function hideStatus() {
    statusDisplay.classList.add('hidden');
}

// Show Success
function showSuccess(response) {
    successDisplay.classList.remove('hidden');

    let details = `<strong>User:</strong> ${response.username || 'Unknown'}<br>`;

    if (response.is_permanent) {
        details += `<strong>License Type:</strong> Permanent<br>`;
        details += `<strong>Status:</strong> Active`;
    } else {
        details += `<strong>Expires:</strong> ${response.expires_at || 'N/A'}<br>`;
        details += `<strong>Time Remaining:</strong> ${response.time_remaining || 'N/A'}`;
    }

    successDetails.innerHTML = details;
}

function hideSuccess() {
    successDisplay.classList.add('hidden');
}

// Form Controls
function disableForm() {
    loginBtn.disabled = true;
    licenseKeyInput.disabled = true;
    rememberMeCheckbox.disabled = true;
    clearBtn.disabled = true;
}

function enableForm() {
    loginBtn.disabled = false;
    licenseKeyInput.disabled = false;
    rememberMeCheckbox.disabled = false;
    clearBtn.disabled = false;
}

// Event Listeners
loginForm.addEventListener('submit', (e) => {
    e.preventDefault();
    handleLogin();
});

clearBtn.addEventListener('click', handleClear);

// Initialize on load
window.addEventListener('DOMContentLoaded', initialize);
