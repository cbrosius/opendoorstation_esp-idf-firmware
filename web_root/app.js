// DOM elements
const statusDot = document.querySelector('.status-dot');
const statusText = document.querySelector('.status-text');
const configForm = document.getElementById('config-form');
const doorbellBtn = document.getElementById('doorbell-btn');
const doorRelayStatus = document.getElementById('door-relay');
const lightRelayStatus = document.getElementById('light-relay');

// Application state
let isConnected = false;
let eventSource = null;

// Initialize application
document.addEventListener('DOMContentLoaded', function() {
    loadConfiguration();
    setupEventListeners();
    setupServerSentEvents();
    updateConnectionStatus();
});

// Setup event listeners
function setupEventListeners() {
    configForm.addEventListener('submit', handleConfigSubmit);
    doorbellBtn.addEventListener('click', handleDoorbellPress);
    
    const factoryResetBtn = document.getElementById('factory-reset-btn');
    if (factoryResetBtn) {
        factoryResetBtn.addEventListener('click', handleFactoryReset);
    }
    
    // Periodic status updates
    setInterval(updateStatus, 2000);
}

// Load current configuration
async function loadConfiguration() {
    try {
        const response = await fetch('/api/config');
        if (response.ok) {
            const config = await response.json();
            populateForm(config);
            updateConnectionStatus(true);
        } else {
            console.error('Failed to load configuration');
            updateConnectionStatus(false);
        }
    } catch (error) {
        console.error('Error loading configuration:', error);
        updateConnectionStatus(false);
    }
}

// Populate form with configuration data
function populateForm(config) {
    document.getElementById('wifi-ssid').value = config.wifi_ssid || '';
    document.getElementById('wifi-password').value = config.wifi_password ? '********' : '';
    document.getElementById('sip-user').value = config.sip_user || '';
    document.getElementById('sip-domain').value = config.sip_domain || '';
    document.getElementById('sip-password').value = config.sip_password ? '********' : '';
    document.getElementById('sip-callee').value = config.sip_callee || '';
    document.getElementById('web-port').value = config.web_port || 80;
    document.getElementById('door-pulse').value = config.door_pulse_duration || 2000;
}

// Handle configuration form submission
async function handleConfigSubmit(event) {
    event.preventDefault();
    
    const formData = new FormData(configForm);
    const config = {};
    
    for (let [key, value] of formData.entries()) {
        // Skip password fields if they contain masked values
        if ((key.includes('password') && value === '********')) {
            continue;
        }
        config[key] = value;
    }
    
    try {
        const response = await fetch('/api/config', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(config)
        });
        
        if (response.ok) {
            showMessage('Configuration saved successfully', 'success');
        } else {
            const error = await response.text();
            showMessage(`Failed to save configuration: ${error}`, 'error');
        }
    } catch (error) {
        console.error('Error saving configuration:', error);
        showMessage('Error saving configuration', 'error');
    }
}

// Handle virtual doorbell press
async function handleDoorbellPress() {
    doorbellBtn.disabled = true;
    doorbellBtn.textContent = '🔔 Calling...';
    
    try {
        const response = await fetch('/api/doorbell', {
            method: 'POST'
        });
        
        if (response.ok) {
            showMessage('Doorbell pressed', 'success');
        } else {
            showMessage('Failed to press doorbell', 'error');
        }
    } catch (error) {
        console.error('Error pressing doorbell:', error);
        showMessage('Error pressing doorbell', 'error');
    } finally {
        setTimeout(() => {
            doorbellBtn.disabled = false;
            doorbellBtn.textContent = '🔔 Virtual Doorbell';
        }, 2000);
    }
}

// Handle factory reset
async function handleFactoryReset() {
    // Show confirmation dialog
    const confirmed = confirm(
        'Are you sure you want to perform a factory reset?\n\n' +
        'This will permanently erase all stored configuration including:\n' +
        '• Wi-Fi credentials\n' +
        '• SIP settings\n' +
        '• All other configuration\n\n' +
        'The device will need to be reconfigured after reset.'
    );
    
    if (!confirmed) {
        return;
    }
    
    const factoryResetBtn = document.getElementById('factory-reset-btn');
    factoryResetBtn.disabled = true;
    factoryResetBtn.textContent = '⚠️ Resetting...';
    
    try {
        const response = await fetch('/api/factory-reset', {
            method: 'POST'
        });
        
        const result = await response.json();
        
        if (response.ok && result.success) {
            showMessage('Factory reset completed successfully', 'success');
            
            // Clear the form after successful reset
            setTimeout(() => {
                configForm.reset();
                showMessage('Configuration form cleared. Please reconfigure the device.', 'info');
            }, 1000);
            
        } else {
            const errorMsg = result.message || 'Factory reset failed';
            showMessage(`Factory reset failed: ${errorMsg}`, 'error');
        }
    } catch (error) {
        console.error('Error performing factory reset:', error);
        showMessage('Error performing factory reset', 'error');
    } finally {
        setTimeout(() => {
            factoryResetBtn.disabled = false;
            factoryResetBtn.textContent = '⚠️ Factory Reset';
        }, 3000);
    }
}

// Update system status
async function updateStatus() {
    try {
        const response = await fetch('/api/status');
        if (response.ok) {
            const status = await response.json();
            updateRelayStatus(status.relays);
            updateConnectionStatus(true);
        } else {
            updateConnectionStatus(false);
        }
    } catch (error) {
        updateConnectionStatus(false);
    }
}

// Update relay status display
function updateRelayStatus(relays) {
    if (relays) {
        updateRelayElement(doorRelayStatus, relays.door);
        updateRelayElement(lightRelayStatus, relays.light);
    }
}

// Update individual relay element
function updateRelayElement(element, state) {
    element.textContent = state ? 'ON' : 'OFF';
    element.className = 'relay-state ' + (state ? 'on' : 'off');
}

// Update connection status indicator
function updateConnectionStatus(connected = false) {
    isConnected = connected;
    
    if (connected) {
        statusDot.classList.add('connected');
        statusText.textContent = 'Connected';
    } else {
        statusDot.classList.remove('connected');
        statusText.textContent = 'Disconnected';
    }
}

// Show temporary message
function showMessage(message, type = 'info') {
    // Create message element
    const messageEl = document.createElement('div');
    messageEl.className = `message message-${type}`;
    messageEl.textContent = message;
    
    // Style the message
    Object.assign(messageEl.style, {
        position: 'fixed',
        top: '20px',
        right: '20px',
        padding: '12px 20px',
        borderRadius: '4px',
        color: 'white',
        fontWeight: '500',
        zIndex: '1000',
        backgroundColor: type === 'success' ? '#27ae60' : 
                        type === 'error' ? '#e74c3c' : '#3498db'
    });
    
    // Add to page
    document.body.appendChild(messageEl);
    
    // Remove after 3 seconds
    setTimeout(() => {
        if (messageEl.parentNode) {
            messageEl.parentNode.removeChild(messageEl);
        }
    }, 3000);
}
// Server-Sent Events functionality
function setupServerSentEvents() {
    const sseUrl = `/events`;
    
    eventSource = new EventSource(sseUrl);
    
    eventSource.onopen = function(event) {
        console.log('SSE connected');
        updateConnectionStatus(true);
    };
    
    eventSource.addEventListener('connected', function(event) {
        console.log('SSE connection confirmed');
        updateConnectionStatus(true);
    });
    
    eventSource.addEventListener('relay_status', function(event) {
        try {
            const message = JSON.parse(event.data);
            handleSSEMessage(message);
        } catch (error) {
            console.error('Error parsing SSE message:', error);
        }
    });
    
    eventSource.onerror = function(event) {
        console.log('SSE disconnected');
        updateConnectionStatus(false);
        
        // EventSource will automatically reconnect
        // But we can add custom logic here if needed
    };
}

function handleSSEMessage(message) {
    console.log('SSE message received:', message);
    
    switch (message.type) {
        case 'relay_status':
            if (message.data && message.data.hasOwnProperty('door') && message.data.hasOwnProperty('light')) {
                updateRelayElement(doorRelayStatus, message.data.door);
                updateRelayElement(lightRelayStatus, message.data.light);
            }
            break;
        
        case 'system_status':
            // Handle system status updates
            console.log('System status update:', message.data);
            break;
        
        default:
            console.log('Unknown SSE message type:', message.type);
    }
}

