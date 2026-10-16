// Medislot API client targeting C++ backend running at http://localhost:8080
export const API_BASE = 'http://localhost:8080';

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
