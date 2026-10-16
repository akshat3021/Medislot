import React, { useState, useEffect } from 'react';
import './style.css';
import { api } from './api';
import AuthLogin from './components/AuthLogin';
import BookingForm from './components/BookingForm';
import Waitlist from './components/Waitlist';
import Dashboard from './components/Dashboard';

const TAB_ROLES = {
  appointments: ['admin', 'reception', 'doctor', 'patient'],
  icu: ['admin', 'reception', 'doctor'],
  dashboard: ['admin', 'reception'],
};

export default function App() {
  const [token, setToken] = useState(() => sessionStorage.getItem('token'));
  const [user, setUser] = useState(() => {
    try {
      return JSON.parse(sessionStorage.getItem('user') || 'null');
    } catch {
      return null;
    }
  });

  const [activeTab, setActiveTab] = useState('appointments');
  const [message, setMessage] = useState({ text: '', isError: false });

  const handleLoginSuccess = (userData, userToken) => {
    setUser(userData);
    setToken(userToken);
    setActiveTab('appointments');
  };

  const handleSignOut = async () => {
    try {
      await api('/logout', 'POST');
    } catch (e) {
      // ignore
    }
    setToken(null);
    setUser(null);
    sessionStorage.clear();
    setMessage({ text: 'Signed out successfully.', isError: false });
  };

  const canAccessTab = (tab) => {
    if (!user || !user.role) return false;
    return TAB_ROLES[tab]?.includes(user.role);
  };

  return (
    <div>
      <header>
        <h1>Medislot Hospital System</h1>
        {user && (
          <span id="who">
            {user.username} ({user.role})
          </span>
        )}
        {user && (
          <button id="logout" onClick={handleSignOut}>
            Sign out
          </button>
        )}
      </header>

      <main>
        {message.text && (
          <p
            id="message"
            role="status"
            className={message.isError ? 'error' : 'ok'}
          >
            {message.text}
          </p>
        )}

        {!token || !user ? (
          <AuthLogin
            onLoginSuccess={handleLoginSuccess}
            setMessage={setMessage}
          />
        ) : (
          <div id="app-view">
            <nav id="tabs">
              {canAccessTab('appointments') && (
                <button
                  type="button"
                  data-tab="appointments"
                  className={activeTab === 'appointments' ? 'active' : ''}
                  onClick={() => setActiveTab('appointments')}
                >
                  Appointments
                </button>
              )}
              {canAccessTab('icu') && (
                <button
                  type="button"
                  data-tab="icu"
                  className={activeTab === 'icu' ? 'active' : ''}
                  onClick={() => setActiveTab('icu')}
                >
                  ICU beds & Waitlist
                </button>
              )}
              {canAccessTab('dashboard') && (
                <button
                  type="button"
                  data-tab="dashboard"
                  className={activeTab === 'dashboard' ? 'active' : ''}
                  onClick={() => setActiveTab('dashboard')}
                >
                  Dashboard
                </button>
              )}
            </nav>

            {activeTab === 'appointments' && canAccessTab('appointments') && (
              <BookingForm user={user} setMessage={setMessage} />
            )}
            {activeTab === 'icu' && canAccessTab('icu') && (
              <Waitlist user={user} setMessage={setMessage} />
            )}
            {activeTab === 'dashboard' && canAccessTab('dashboard') && (
              <Dashboard user={user} setMessage={setMessage} />
            )}
          </div>
        )}
      </main>
    </div>
  );
}
