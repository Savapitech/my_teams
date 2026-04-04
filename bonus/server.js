const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const net = require('net');
const path = require('path');

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

app.use(express.static(path.join(__dirname, 'public')));

const MTP_PORT = process.env.MTP_PORT || 5290;
const MTP_HOST = process.env.MTP_HOST || '127.0.0.1';

wss.on('connection', (ws) => {
    console.log('[Web Proxy] New Web Client connected');
    
    const tcpClient = new net.Socket();
    
    tcpClient.connect(MTP_PORT, MTP_HOST, () => {
        console.log(`[Web Proxy] Connected to MTP Server at ${MTP_HOST}:${MTP_PORT}`);
        ws.send(JSON.stringify({ type: 'system', message: `Connected to MTP Server on port ${MTP_PORT}` }));
    });

    tcpClient.on('data', (data) => {
        const msgs = data.toString().split('\n').filter(m => m.trim().length > 0);
        msgs.forEach(msg => {
            if (ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ type: 'server', message: msg }));
            }
        });
    });

    tcpClient.on('close', () => {
        console.log('[Web Proxy] TCP connection closed');
        if (ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({ type: 'system', message: 'MTP Server connection closed' }));
            ws.close();
        }
    });

    tcpClient.on('error', (err) => {
        console.error('[Web Proxy] TCP Error:', err.message);
        if (ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({ type: 'error', message: 'MTP Server error: ' + err.message }));
        }
    });

    ws.on('message', (message) => {
        const cmd = message.toString().trim();
        console.log(`[Web Proxy] Web -> TCP : ${cmd}`);
        tcpClient.write(cmd + '\n');
    });

    ws.on('close', () => {
        console.log('[Web Proxy] Web Client disconnected');
        tcpClient.destroy();
    });
});

const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
    console.log(`Web client server is running on http://localhost:${PORT}`);
    console.log(`Proxying WebSockets to TCP -> ${MTP_HOST}:${MTP_PORT}`);
});