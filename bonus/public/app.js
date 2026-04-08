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
}

function showloginscreen(username) {
    document.getElementById('appScreen').style.display = 'none';
    document.getElementById('loginScreen').style.display = 'flex';
}

let tryingLogin = false;
let teams = [];

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
          uuid: msgMatch[1], name: msgMatch[2], desc: msgMatch[3]
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
    //appendLog(data.message, data.type);
};

ws.onclose = () => {
    appendLog('Disconnected from WebSocket proxy.', 'error');
};


function displayTeams() {
  const sidemenu = document.querySelector('.sidemenu');
   
  sidemenu.innerHTML = '<h1>Teams :</h1>';

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
          channels.appendChild(eldiv);
        });
        div.appendChild(channels);
        
        div.addEventListener('click', () => {
            ws.send(`USE "${team.uuid}"`);
            ws.send('LIST');
        }); 
        sidemenu.appendChild(div);
    });
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
