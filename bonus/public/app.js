const ws = new WebSocket(`ws://${window.location.host}`);
const log = document.getElementById('log');
const input = document.getElementById('cmdInput');
const sendBtn = document.getElementById('sendBtn');
const username = sessionStorage.getItem('username');

let tryingLogin = false;
let teams = [];
let currentTeamUUID = null;
let currentChannelUUID = null;
let currentThreadUUID = null;


if (username) {
    showappscreen(username);
} else {
    showloginscreen();
}

function showappscreen(username) {
    document.getElementById('loginScreen').style.display = 'none';
    document.getElementById('appScreen').style.display = 'flex';
}

function showloginscreen(username) {
    document.getElementById('appScreen').style.display = 'none';
    document.getElementById('loginScreen').style.display = 'flex';
}


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
        ws.send ('LIST');
    }

setInterval(() => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send('LIST');
    }
  }, 5000);  
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
    if (data.message.startsWith('Team UUID:')) {
      const msgMatch = data.message.match(/Team UUID: (.*), Name: (.*), Desc: (.*)/);
      if (msgMatch) {
        const team = {
          uuid: msgMatch[1], name: msgMatch[2], desc: msgMatch[3], channels: []
        };
        let inside = false;
        for (var i = 0; i < teams.length; i++) {
          if (teams[i].uuid == team.uuid)
            inside = true;
        }
        if (inside == false) {
          teams.push(team);
          displayTeams();
        }
      }
    }
    if (data.message.startsWith('Channel UUID:')) {
      const msgMatch = data.message.match(/Channel UUID: (.*), Name: (.*), Desc: (.*), Team UUID: (.*)/);
      if (msgMatch) {
        const channel = {
          uuid: msgMatch[1], name: msgMatch[2], desc: msgMatch[3], threads: []
        };
        const teamID = msgMatch[4];
        const team = teams.find(t => t.uuid == teamID);
        if (team) {
          const dup = team.channels.some(c => c.uuid == channel.uuid);
          if (!dup) {
            team.channels.push(channel);
            displayTeams();
          }
        }
      }
    }
    if (data.message.startsWith('Thread UUID:')) {
      const msgMatch = data.message.match(/Thread UUID: (.*), Title: (.*), Body: (.*), Channel UUID: (.*)/);
      if (msgMatch) {
        displayThread({ uuid: msgMatch[1], name: msgMatch[2], desc: msgMatch[3] });
      }
    } 
    console.log(data.message);
    //appendLog(data.message, data.type);
};

ws.onclose = () => {
    appendLog('Disconnected from WebSocket proxy.', 'error');
};


function displayTeams() {
  const sidemenu = document.querySelector('.sidemenu');
   
  sidemenu.innerHTML = '<h1>My Teams</h1>';

  teams.forEach(team => {
      const div = document.createElement('div');
      div.className = 'team-container';
      div.textContent = " - " + team.name;

      const channels = document.createElement('div');
      channels.className = 'channel-container';
      team.channels.forEach(el => {
        const eldiv = document.createElement('div');
        eldiv.className = 'el-container';
        eldiv.textContent = " ¤ " + el.name; 
          
        eldiv.addEventListener('click', (it) => {
          it.stopPropagation();
          currentChannelUUID = el.uuid;
          ws.send(`USE "${team.uuid}" "${el.uuid}"`);
          ws.send(`LIST`);
        document.getElementById('thread-header').textContent = " ~ " + el.name;
        document.getElementById('thread-container').innerHTML = '';
        });
        channels.appendChild(eldiv);
      });

      div.addEventListener('click', () => {
          currentTeamUUID = team.uuid;
          ws.send(`USE "${team.uuid}"`);
          ws.send ('LIST');
      });
      div.appendChild(channels);
      sidemenu.appendChild(div);
  });
}

function displayThread(thread) {
  console.log(thread);
  const items = document.getElementById('thread-container');
    if (document.querySelector(`[data-thread-id="${thread.uuid}"]`)) return;
      const div = document.createElement('div');
    div.className = 'thread';
    div.dataset.threadId = thread.uuid;
    div.innerHTML = `
      <div class="thread-name">${thread.name}</div>
      <div class="thread-desc">${thread.desc}</div>
    `;
    div.addEventListener('click', () => {
      document.querySelectorAll('.thread').forEach(el => el.classList.remove('active'));
      div.classList.add('active');
      currentThreadUUID = thread.uuid;
      ws.send(`USE "${currentTeamUUID}" "${currentChannelUUID}" "${thread.uuid}"`);
      ws.send('LIST');
    });
    items.appendChild(div);
}


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
