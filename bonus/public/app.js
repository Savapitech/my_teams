const ws = new WebSocket(`ws://${window.location.host}`);
const log = document.getElementById('log');
const input = document.getElementById('cmdInput');
const sendBtn = document.getElementById('sendBtn');
const username = sessionStorage.getItem('username');

if (username) {
    showappscreen(username);
} else {
    showloginscreen();
}

function showappscreen(username) {
    document.getElementById('loginScreen').style.display = 'none';
    document.getElementById('appScreen').style.display = 'flex';
    //document.getElementById('userName').textContent = username;
}

function showloginscreen(username) {
    document.getElementById('appScreen').style.display = 'none';
    document.getElementById('loginScreen').style.display = 'flex';
}

let tryingLogin = false;

function Login() {
    const username = document.getElementById('usernameInput').value.trim();
    if (!username) 
        return;
    tryingLogin = true;
    ws.send(`LOGIN "${username}"`);
}

document.getElementById('loginBtn').addEventListener('click', Login);
document.getElementById('usernameInput').addEventListener('keypress', (e) => {
    if (e.key === 'Enter') Login();
});

function logout() {
    sessionStorage.removeItem('username');
    ws.send('LOGOUT');
    showloginscreen();
}

document.getElementById('logoutBtn').addEventListener('click', logout);

function appendLog(text, type) {
    const div = document.createElement('div');
    div.className = `msg ${type}`;
    div.textContent = text;
    log.appendChild(div);
    log.scrollTop = log.scrollHeight;
}

ws.onopen = () => {
    appendLog('Connected to WebSocket proxy.', 'system');
    if (username) {
        ws.send(`LOGIN "${username}"`);
    }
};

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    if (tryingLogin) {
        tryingLogin = false;
        if (data.message.startsWith('200')) {
            const username = document.getElementById('usernameInput').value.trim();
            sessionStorage.setItem('username', username);
            showappscreen(username);
        } else {
            document.getElementById('login-error').style.display = 'block';
        }
        return;
    }
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
