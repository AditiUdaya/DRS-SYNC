import axios from 'axios';

const API_BASE = process.env.REACT_APP_API_URL || 'http://localhost:8080/api';

export const api = {
  // Files
  uploadFile: async (file, priority = 'standard') => {
    const formData = new FormData();
    formData.append('file', file);
    formData.append('priority', priority);
    
    const response = await axios.post(`${API_BASE}/files/upload`, formData, {
      headers: { 'Content-Type': 'multipart/form-data' }
    });
    return response.data;
  },

  listFiles: async () => {
    const response = await axios.get(`${API_BASE}/files`);
    return response.data;
  },

  getFileStatus: async (fileId) => {
    const response = await axios.get(`${API_BASE}/files/${fileId}`);
    return response.data;
  },

  startTransfer: async (fileId, destinationHost, destinationPort) => {
    const response = await axios.post(`${API_BASE}/files/${fileId}/transfer`, {
      file_id: fileId,
      destination_host: destinationHost,
      destination_port: destinationPort
    });
    return response.data;
  },

  pauseTransfer: async (fileId) => {
    const response = await axios.post(`${API_BASE}/files/${fileId}/pause`);
    return response.data;
  },

  resumeTransfer: async (fileId) => {
    const response = await axios.post(`${API_BASE}/files/${fileId}/resume`);
    return response.data;
  },

  cancelTransfer: async (fileId) => {
    const response = await axios.post(`${API_BASE}/files/${fileId}/cancel`);
    return response.data;
  },

  updatePriority: async (fileId, priority) => {
    const response = await axios.put(`${API_BASE}/files/${fileId}/priority`, {
      priority
    });
    return response.data;
  },

  // Links
  getLinks: async () => {
    const response = await axios.get(`${API_BASE}/links`);
    return response.data;
  },

  getBestLink: async () => {
    const response = await axios.get(`${API_BASE}/links/best`);
    return response.data;
  },

  // Simulator
  setSimulatorConfig: async (config) => {
    const response = await axios.post(`${API_BASE}/simulator/config`, config);
    return response.data;
  },

  killLink: async (interfaceName = null) => {
    const response = await axios.post(`${API_BASE}/simulator/kill-link`, null, {
      params: { interface: interfaceName }
    });
    return response.data;
  },

  restoreLink: async (interfaceName = null) => {
    const response = await axios.post(`${API_BASE}/simulator/restore-link`, null, {
      params: { interface: interfaceName }
    });
    return response.data;
  },

  resetSimulator: async (interfaceName = null) => {
    const response = await axios.post(`${API_BASE}/simulator/reset`, null, {
      params: { interface: interfaceName }
    });
    return response.data;
  }
};

