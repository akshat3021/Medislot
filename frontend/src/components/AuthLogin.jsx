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
    <div className="login-page">
      <div className="login-card">
        {/* Card header */}
        <div className="login-card-header">
          <div className="login-brand">
            <span className="login-brand-icon" aria-hidden="true">+</span>
            <span className="login-brand-name">Medislot</span>
          </div>
          <h2 className="login-title" style={{ margin: 0, fontSize: '1.15rem' }}>
            Sign in to your account
          </h2>
          <p className="login-subtitle" style={{ marginTop: '0.25rem' }}>
            Authorized clinical and patient access only
          </p>
        </div>

        {/* Login form */}
        <div className="login-card-body">
          <form id="login-form" onSubmit={handleSubmit} noValidate>
            <label htmlFor="username">
              <span className="label-text">Username</span>
              <input
                id="username"
                type="text"
                value={username}
                onChange={(e) => setUsername(e.target.value)}
                required
                autoComplete="username"
                autoFocus
                placeholder="e.g. admin, drmehta, alice"
              />
            </label>

            <label htmlFor="password">
              <span className="label-text">Password</span>
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

            <button
              type="submit"
              className="login-submit-btn"
              disabled={loading}
            >
              {loading ? 'Signing in…' : 'Sign in'}
            </button>
          </form>
        </div>

        {/* Quick demo accounts */}
        <div className="login-demo">
          <div className="login-demo-label">Quick demo accounts</div>
          <div className="quick-roles-container">
            <button
              type="button"
              className="quick-role-btn"
              onClick={() => handleQuickLogin('admin', 'admin123')}
            >
              Admin
            </button>
            <button
              type="button"
              className="quick-role-btn"
              onClick={() => handleQuickLogin('reception', 'reception123')}
            >
              Reception
            </button>
            <button
              type="button"
              className="quick-role-btn"
              onClick={() => handleQuickLogin('drmehta', 'doctor123')}
            >
              Dr. Mehta
            </button>
            <button
              type="button"
              className="quick-role-btn"
              onClick={() => handleQuickLogin('alice', 'patient123')}
            >
              Alice
            </button>
            <button
              type="button"
              className="quick-role-btn"
              onClick={() => handleQuickLogin('bob', 'patient123')}
            >
              Bob
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}
