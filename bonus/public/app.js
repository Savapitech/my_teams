const ws = new WebSocket(`ws://${window.location.host}`);
const log = document.getElementById('log');
const input = document.getElementById('cmdInput');
const sendBtn = document.getElementById('sendBtn');

function appendLog(text, type) {
    const div = document.createElement('div');
    div.className = `msg ${type}`;
    div.textContent = text;
    log.appendChild(div);
    log.scrollTop = log.scrollHeight;
}

ws.onopen = () => {
    appendLog('Connected to WebSocket proxy.', 'system');
};

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    appendLog(data.message, data.type);
};

ws.onclose = () => {
    appendLog('Disconnected from WebSocket proxy.', 'error');
};

function sendCmd() {
    const cmd = input.value.trim();
    if (cmd) {
        appendLog(cmd, 'client');
        ws.send(cmd);
        input.value = '';
    }
}

sendBtn.addEventListener('click', sendCmd);
input.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') sendCmd();
});

input.focus();