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
      <form id="login-form" onSubmit={handleSubmit}>
        <label>
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
        <label>
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
        <button type="submit" disabled={loading}>
          {loading ? 'Signing in...' : 'Sign in'}
        </button>
      </form>

      <div style={{ marginTop: '1rem', paddingTop: '1rem', borderTop: '1px solid #e5e7eb' }}>
        <p style={{ margin: '0 0 .5rem', fontSize: '0.85rem', color: '#6b7280' }}>
          <strong>Quick Role Demo Accounts:</strong>
        </p>
        <div style={{ display: 'flex', gap: '0.5rem', flexWrap: 'wrap' }}>
          <button type="button" onClick={() => handleQuickLogin('admin', 'admin123')} style={{ fontSize: '0.8rem', padding: '0.3rem 0.6rem' }}>
            Admin (Full Access)
          </button>
          <button type="button" onClick={() => handleQuickLogin('reception', 'reception123')} style={{ fontSize: '0.8rem', padding: '0.3rem 0.6rem' }}>
            Reception (Staff)
          </button>
          <button type="button" onClick={() => handleQuickLogin('drmehta', 'doctor123')} style={{ fontSize: '0.8rem', padding: '0.3rem 0.6rem' }}>
            Doctor (Appointments/ICU)
          </button>
          <button type="button" onClick={() => handleQuickLogin('alice', 'patient123')} style={{ fontSize: '0.8rem', padding: '0.3rem 0.6rem' }}>
            Patient (Alice)
          </button>
          <button type="button" onClick={() => handleQuickLogin('bob', 'patient123')} style={{ fontSize: '0.8rem', padding: '0.3rem 0.6rem' }}>
            Patient (Bob)
          </button>
        </div>
      </div>
    </section>
  );
}
