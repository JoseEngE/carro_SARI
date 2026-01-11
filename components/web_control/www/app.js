// HTTP polling connection
let sendInterval = null;
let telemetryInterval = null;
let pingInterval = null;
let lastPingTime = 0;
let isConnected = false;

// Joystick state
let steeringValue = 0;
let throttleValue = 0;

// DOM elements
const statusElement = document.getElementById('status');
const statusText = statusElement.querySelector('.status-text');
const latencyElement = document.getElementById('latency');
const batteryElement = document.getElementById('battery');
const speedElement = document.getElementById('speed');
const stopButton = document.getElementById('stop-button');

// Joystick elements
const steeringJoystick = document.getElementById('steering-joystick');
const steeringKnob = document.getElementById('steering-knob');
const throttleJoystick = document.getElementById('throttle-joystick');
const throttleKnob = document.getElementById('throttle-knob');

// Initialize connection
function startConnection() {
    isConnected = true;
    updateStatus(true);
    startSendLoop();
    startTelemetryLoop();
    startPing();
}

// Update connection status
function updateStatus(connected) {
    isConnected = connected;
    if (connected) {
        statusElement.classList.add('connected');
        statusText.textContent = 'Connected';
    } else {
        statusElement.classList.remove('connected');
        statusText.textContent = 'Disconnected';
        latencyElement.textContent = '--ms';
    }
}

// Start ping/heartbeat
function startPing() {
    pingInterval = setInterval(async () => {
        try {
            lastPingTime = Date.now();
            const response = await fetch('/telemetry');
            if (response.ok) {
                const latency = Date.now() - lastPingTime;
                latencyElement.textContent = `${latency}ms`;
                if (!isConnected) {
                    startConnection();
                }
            }
        } catch (error) {
            console.error('Ping failed:', error);
            updateStatus(false);
        }
    }, 1000);
}

// Send motor commands at 50Hz (20ms)
function startSendLoop() {
    sendInterval = setInterval(() => {
        sendMotorCommand();
    }, 20); // 50Hz = 20ms
}

function stopSendLoop() {
    if (sendInterval) {
        clearInterval(sendInterval);
        sendInterval = null;
    }
}

// Fetch telemetry data
function startTelemetryLoop() {
    telemetryInterval = setInterval(async () => {
        try {
            const response = await fetch('/telemetry');
            if (response.ok) {
                const data = await response.json();
                batteryElement.textContent = `${data.battery}%`;
                speedElement.textContent = `${data.speed.toFixed(1)} km/h`;
            }
        } catch (error) {
            console.error('Telemetry fetch failed:', error);
        }
    }, 100); // Update every 100ms
}

// Send motor command via HTTP POST
async function sendMotorCommand() {
    if (!isConnected) return;

    try {
        const msg = new Uint8Array(4);
        msg[0] = 0x01; // Motor control message
        msg[1] = throttleValue; // Throttle (-100 to +100)
        msg[2] = steeringValue; // Steering (-100 to +100)
        msg[3] = (msg[1] + msg[2]) & 0xFF; // Simple checksum

        await fetch('/command', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/octet-stream'
            },
            body: msg
        });
    } catch (error) {
        console.error('Command send failed:', error);
        updateStatus(false);
    }
}

// Emergency stop
async function emergencyStop() {
    steeringValue = 0;
    throttleValue = 0;
    updateJoystickPosition(steeringKnob, 0, 0);
    updateJoystickPosition(throttleKnob, 0, 0);

    try {
        const msg = new Uint8Array([0x03]); // Emergency stop
        await fetch('/command', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/octet-stream'
            },
            body: msg
        });
    } catch (error) {
        console.error('Emergency stop failed:', error);
    }
}

// Joystick handling
function setupJoystick(joystickElement, knobElement, isVertical, callback) {
    let isDragging = false;
    let startX = 0;
    let startY = 0;
    let currentX = 0;
    let currentY = 0;

    const maxDistance = 40; // Maximum distance from center

    function handleStart(e) {
        isDragging = true;
        const touch = e.touches ? e.touches[0] : e;
        const rect = joystickElement.getBoundingClientRect();
        startX = rect.left + rect.width / 2;
        startY = rect.top + rect.height / 2;
        handleMove(e);
    }

    function handleMove(e) {
        if (!isDragging) return;

        e.preventDefault();
        const touch = e.touches ? e.touches[0] : e;

        let deltaX = touch.clientX - startX;
        let deltaY = touch.clientY - startY;

        // Limit to max distance
        const distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
        if (distance > maxDistance) {
            const angle = Math.atan2(deltaY, deltaX);
            deltaX = Math.cos(angle) * maxDistance;
            deltaY = Math.sin(angle) * maxDistance;
        }

        currentX = deltaX;
        currentY = deltaY;

        updateJoystickPosition(knobElement, deltaX, deltaY);

        // Calculate value (-100 to +100)
        let value;
        if (isVertical) {
            value = Math.round((-deltaY / maxDistance) * 100);
        } else {
            value = Math.round((deltaX / maxDistance) * 100);
        }

        // Clamp value
        value = Math.max(-100, Math.min(100, value));
        callback(value);
    }

    function handleEnd(e) {
        if (!isDragging) return;
        isDragging = false;

        // Return to center with animation
        currentX = 0;
        currentY = 0;
        updateJoystickPosition(knobElement, 0, 0);
        callback(0);
    }

    // Touch events
    joystickElement.addEventListener('touchstart', handleStart, { passive: false });
    document.addEventListener('touchmove', handleMove, { passive: false });
    document.addEventListener('touchend', handleEnd);

    // Mouse events (for desktop testing)
    joystickElement.addEventListener('mousedown', handleStart);
    document.addEventListener('mousemove', handleMove);
    document.addEventListener('mouseup', handleEnd);
}

function updateJoystickPosition(knob, x, y) {
    knob.style.transform = `translate(calc(-50% + ${x}px), calc(-50% + ${y}px))`;
}

// Initialize joysticks
setupJoystick(steeringJoystick, steeringKnob, false, (value) => {
    steeringValue = value;
});

setupJoystick(throttleJoystick, throttleKnob, true, (value) => {
    throttleValue = value;
});

// Stop button
stopButton.addEventListener('click', emergencyStop);
stopButton.addEventListener('touchstart', (e) => {
    e.preventDefault();
    emergencyStop();
});

// Prevent scrolling and zooming
document.addEventListener('touchmove', (e) => {
    if (e.target.closest('.joystick') || e.target.closest('.stop-button')) {
        e.preventDefault();
    }
}, { passive: false });

// Initialize connection on load
window.addEventListener('load', () => {
    startConnection();
});

// Cleanup on unload
window.addEventListener('beforeunload', () => {
    stopSendLoop();
    if (telemetryInterval) clearInterval(telemetryInterval);
    if (pingInterval) clearInterval(pingInterval);
});
