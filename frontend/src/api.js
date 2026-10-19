// Medislot API client targeting C++ backend
// If VITE_API_URL is configured (e.g. for Vercel pointing to Render), use that;
// otherwise fall back to relative path '' if served from same origin (Render), or localhost for dev.
export const API_BASE = import.meta.env.VITE_API_URL || (typeof window !== 'undefined' && window.location.hostname !== 'localhost' ? '' : 'http://localhost:8080');

export async function api(path, method = 'GET', body = null, onUnauthorized = null) {
  const token = sessionStorage.getItem('token');
  const headers = { 'Content-Type': 'application/json' };
  if (token) {
    headers['Authorization'] = 'Bearer ' + token;
  }

  const response = await fetch(`${API_BASE}/api${path}`, {
    method,
    headers,
    body: body ? JSON.stringify(body) : undefined,
  });

  const data = await response.json().catch(() => ({}));

  if (response.status === 401 && token) {
    if (onUnauthorized) {
      onUnauthorized();
    }
  }

  if (!response.ok) {
    throw new Error(data.error || 'Something went wrong');
  }

  return data;
}
