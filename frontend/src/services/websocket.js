let ws = null;
let reconnectInterval = null;
let messageHandlers = [];

export function connectWebSocket(onMessage) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    return ws;
  }

  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  const wsUrl = `${protocol}//${window.location.host.replace(':3000', ':8080')}/ws`;
  
  ws = new WebSocket(wsUrl);

  ws.onopen = () => {
    console.log('WebSocket connected');
    if (reconnectInterval) {
      clearInterval(reconnectInterval);
      reconnectInterval = null;
    }
  };

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if (onMessage) {
        onMessage(data);
      }
      messageHandlers.forEach(handler => handler(data));
    } catch (error) {
      console.error('Error parsing WebSocket message:', error);
    }
  };

  ws.onerror = (error) => {
    console.error('WebSocket error:', error);
  };

  ws.onclose = () => {
    console.log('WebSocket disconnected');
    // Attempt to reconnect
    if (!reconnectInterval) {
      reconnectInterval = setInterval(() => {
        connectWebSocket(onMessage);
      }, 3000);
    }
  };

  return ws;
}

export function disconnectWebSocket(wsInstance) {
  if (wsInstance) {
    wsInstance.close();
  }
  if (reconnectInterval) {
    clearInterval(reconnectInterval);
    reconnectInterval = null;
  }
}

export function addMessageHandler(handler) {
  messageHandlers.push(handler);
}

export function removeMessageHandler(handler) {
  messageHandlers = messageHandlers.filter(h => h !== handler);
}

