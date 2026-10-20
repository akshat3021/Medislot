import React, { useState } from 'react';
import { api } from '../api';

export default function AuthLogin({ onLoginSuccess, setMessage }) {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setLoading(true);
    setMessage({ text: '', isError: false });

    try {
      const data = await api('/login', 'POST', { username, password });
      sessionStorage.setItem('token', data.token);
      sessionStorage.setItem('user', JSON.stringify(data));
      onLoginSuccess(data, data.token);
      setMessage({ text: `Welcome back, ${data.username}!`, isError: false });
    } catch (err) {
      setMessage({ text: err.message, isError: true });
    } finally {
      setLoading(false);
    }
  };

  const handleQuickLogin = (user, pass) => {
    setUsername(user);
    setPassword(pass);
  };

  return (
    <section id="login-view">
      <h2>Sign in to Medislot</h2>
      <p style={{ margin: '0 0 1.25rem', fontSize: '0.9rem', color: 'var(--text-muted)' }}>
        Authorized clinical and patient access portal.
      </p>

      <form id="login-form" onSubmit={handleSubmit} style={{ margin: 0, padding: 0, border: 'none', boxShadow: 'none' }}>
        <label htmlFor="username">
          Username
          <input
            id="username"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            required
            autoComplete="username"
            placeholder="e.g. admin, reception, drmehta, alice"
          />
        </label>
        <label htmlFor="password">
          Password
          <input
            id="password"
            type="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            required
            autoComplete="current-password"
            placeholder="Enter password"
          />
        </label>
        <div style={{ marginTop: '1.25rem' }}>
          <button type="submit" disabled={loading}>
            {loading ? 'Signing in...' : 'Sign in'}
          </button>
        </div>
      </form>

      <div style={{ marginTop: '1.5rem', paddingTop: '1.25rem', borderTop: '1px solid var(--border-subtle)' }}>
        <p style={{ margin: '0 0 0.5rem', fontSize: '0.85rem', fontWeight: 600, color: 'var(--text-secondary)' }}>
          Quick Role Demo Accounts:
        </p>
        <div className="quick-roles-container">
          <button
            type="button"
            className="quick-role-btn"
            onClick={() => handleQuickLogin('admin', 'admin123')}
          >
            Admin (Full Access)
          </button>
          <button
            type="button"
            className="quick-role-btn"
            onClick={() => handleQuickLogin('reception', 'reception123')}
          >
            Reception (Staff)
          </button>
          <button
            type="button"
            className="quick-role-btn"
            onClick={() => handleQuickLogin('drmehta', 'doctor123')}
          >
            Doctor (Appointments/ICU)
          </button>
          <button
            type="button"
            className="quick-role-btn"
            onClick={() => handleQuickLogin('alice', 'patient123')}
          >
            Patient (Alice)
          </button>
          <button
            type="button"
            className="quick-role-btn"
            onClick={() => handleQuickLogin('bob', 'patient123')}
          >
            Patient (Bob)
          </button>
        </div>
      </div>
    </section>
  );
}
