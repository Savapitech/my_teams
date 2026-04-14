const ws = new WebSocket(`ws://${window.location.host}`);
const log = document.getElementById('log');
const input = document.getElementById('cmdInput');
const sendBtn = document.getElementById('sendBtn');
const username = sessionStorage.getItem('username');

let userID = null;
let tryingLogin = false;
let teams = [];
let currentTeamUUID = null;
let currentChannelUUID = null;
let currentThreadUUID = null;
const messages = new Set();

if (username) {
    showappscreen(username);
} else {
    showloginscreen();
}

function showappscreen(username) {
    document.getElementById('loginScreen').style.display = 'none';
    document.getElementById('appScreen').style.display = 'flex';
}

function showloginscreen() {
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
    const values = [...text.matchAll(/"([^"]*)"/g)].map(m => m[1]);
    while (values.length >= 4) {
      const div = document.createElement('div');
      const uid = values[1];
      const timestamp = values[2];
      const message = values[3];
      const date = new Date(timestamp * 1000).toLocaleDateString("fr-FR", {
        day: '2-digit',
        month: '2-digit',
        hour: '2-digit',
        minute: '2-digit'
      });
      if (uid == userID) {
        div.className = "msg client";
      } else {
        div.className = "msg server";
      }
      div.textContent = `${date} \n ${message}`;
      log.appendChild(div);
      log.scrollTop = log.scrollHeight;
      for(var x = 0; x < 4; x++)
        values.shift();
        
    }
}

ws.onopen = () => {
    appendLog('Connected to WebSocket proxy.', 'system');
    if (username) {
        ws.send(`LOGIN "${username}"`);
        ws.send('INFO');
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
        if (data.message.startsWith('200 Logged in')) {
            const username = document.getElementById('usernameInput').value.trim();
            sessionStorage.setItem('username', username);
            showappscreen(username);
        } else {
            document.getElementById('login-error').style.display = 'block';
        }
        return;
    }
    if (data.message.startsWith('200 OK TEAM')) {
      const values = [...data.message.matchAll(/"([^"]*)"/g)].map(m => m[1]);

      while (values.length >= 3) {
        const team = {
        uuid: values[0],
        name: values[1],
        desc: values[2],
        channels: []
        };

        let inside = false;
        for (var i = 0; i < teams.length; i++) {
          if (teams[i].uuid == team.uuid)
            inside = true;
        }

        if (!inside) {
          teams.push(team);
          displayTeams();
        }
      for (var x = 0; x < 3; x++)
        values.shift();
      }
    }
    if (data.message.startsWith('200 OK CHANNEL')) {
      const values = [...data.message.matchAll(/"([^"]*)"/g)].map(m => m[1]);
      const team = teams.find(t => t.uuid == currentTeamUUID);

      while (values.length >= 3 && team) {
        const channel = {
          uuid: values[0],
          name: values[1],
          desc: values[2],
          threads: []
        };

        const dup = team.channels.some(c => c.uuid == channel.uuid);
        if (!dup) {
          team.channels.push(channel);
          displayTeams();
        }

        for (var x = 0; x < 3; x++)
          values.shift();
      }
    }
    if (data.message.startsWith('200 OK THREAD')) {
      const values = [...data.message.matchAll(/"([^"]*)"/g)].map(m => m[1]); 
      while (values.length >= 5) {
        displayThread({uuid: values[0], name: values[3], desc:values[4]})
        for (var x = 0; x < 5; x++)
          values.shift();
      }
    }
    
    console.log(data.message);
    if (data.message.startsWith('200 OK REPLY')) {
      if (messages.has(data.message)) return;
      messages.add(data.message);
      appendLog(data.message, data.type);
    }

    if (data.message.startsWith('200 OK USER')) {
      const values = [...data.message.matchAll(/"([^"]*)"/g)].map(m => m[1]); 
      if (values) {
        userID = values[0];
      }
    }
};

ws.onclose = () => {
    appendLog('Disconnected from WebSocket proxy.', 'error');
};


function displayTeams() {
  const sidemenu = document.querySelector('.sidemenu');
   
  sidemenu.querySelectorAll('.team-container').forEach(el => el.remove());

  if (!sidemenu.querySelector('.sidemenu-title')) {
    const title = document.createElement('h1');
    title.className = 'sidemenu-title';
    title.textContent = 'My Teams';
    sidemenu.insertBefore(title, sidemenu.querySelector('.create-bar'));
  }

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
          currentTeamUUID = team.uuid;
          ws.send(`USE "${team.uuid}" "${el.uuid}"`);
          ws.send(`LIST`);
        document.getElementById('thread-header').textContent = " ~ " + el.name;
        document.getElementById('thread-container').innerHTML = '';
        createChannelBtn.disabled = false;
        createThreadBtn.disabled = false;
        });
        channels.appendChild(eldiv);
      });

      div.addEventListener('click', () => {
          currentTeamUUID = team.uuid;
          ws.send(`USE "${team.uuid}"`);
          ws.send ('LIST');
          createChannelBtn.disabled = false;
          createThreadBtn.disabled = true;
      });
      div.appendChild(channels);
      sidemenu.appendChild(div);
  });
}

function displayThread(thread) {
  console.log("inside" + thread);
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
      messages.clear();
      ws.send(`USE "${currentTeamUUID}" "${currentChannelUUID}" "${thread.uuid}"`);
      ws.send('LIST');
    });
    items.appendChild(div);
}


function sendCmd() {
    const cmd = input.value.trim();
    if (cmd) {
        ws.send(`CREATE "${cmd}"`);
        input.value = '';
        ws.send("LIST");
    }
}

sendBtn.addEventListener('click', sendCmd);
input.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') sendCmd();
});

input.focus();

let createMode = null;

const createTeamBtn = document.getElementById('create-team-btn');
const createChannelBtn = document.getElementById('create-channel-btn');
const createThreadBtn = document.getElementById('create-thread-btn');
const createForm = document.getElementById('create-form');
const createName = document.getElementById('create-name');
const createDesc = document.getElementById('create-desc');
const createConfirmBtn = document.getElementById('create-confirm-btn');
const createCancelBtn  = document.getElementById('create-cancel-btn');

function openCreateForm(mode) {
  createMode = mode;
  createName.placeholder = mode === 'thread' ? 'Thread title' : 'Name';
  createDesc.style.display = mode === 'thread' ? 'none' : 'block';
  createDesc.placeholder = mode === 'team' ? 'Team description' : 'Channel description';
  createName.value = '';
  createDesc.value = '';
  createForm.style.display = 'block';
  createName.focus();
}

createTeamBtn.addEventListener('click', () => openCreateForm('team'));
createChannelBtn.addEventListener('click', () => openCreateForm('channel'));
createThreadBtn.addEventListener('click', () => openCreateForm('thread'));
createCancelBtn.addEventListener('click', () => { createForm.style.display = 'none'; createMode = null; });

createConfirmBtn.addEventListener('click', () => {
  const name = createName.value.trim();
  const desc = createDesc.value.trim();
  if (!name) return;

  if (createMode === 'team') {
    ws.send(`USE`);
    ws.send(`CREATE "${name}" "${desc || name}"`);
    setTimeout(() => {
      ws.send('USE');
      ws.send('LIST');
    }, 300);

  } else if (createMode === 'channel') {
    console.log("channel " + currentTeamUUID);
    if (!currentTeamUUID) return;
    console.log(`log : USE "${currentTeamUUID}"`);
    ws.send(`USE "${currentTeamUUID}"`);
    ws.send(`SUBSCRIBE "${currentTeamUUID}"`);
    console.log(`log : CREATE "${name}" "${desc}"`);
    ws.send('INFO');
    ws.send(`CREATE "${name}" "${desc}"`);

  } else if (createMode === 'thread') {
    console.log("thread " + currentTeamUUID + " " + currentChannelUUID);
    if (!currentTeamUUID || !currentChannelUUID) return;
    console.log(`USE "${currentTeamUUID}" "${currentChannelUUID}"`);
    console.log(`CREATE "${name}"`);
    ws.send(`USE "${currentTeamUUID}" "${currentChannelUUID}"`);
    ws.send(`CREATE "${name}" "${name}"`);
  }
  ws.send('LIST');
  createForm.style.display = 'none';
  createMode = null;
});

createName.addEventListener('keypress', (e) => { if (e.key === 'Enter') createConfirmBtn.click(); });
