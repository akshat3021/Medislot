import React, { useState } from 'react';
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
      // ignore network errors on signout
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
        <div className="header-brand">
          <span className="brand-icon" aria-hidden="true">+</span>
          <h1>Medislot Hospital System</h1>
        </div>

        {user && (
          <div className="header-user-nav">
            <span id="who" title={`Logged in as ${user.username} (${user.role})`}>
              {user.username} &middot; {user.role}
            </span>
            <button id="logout" type="button" onClick={handleSignOut}>
              Sign out
            </button>
          </div>
        )}
      </header>

      <main>
        {/* Notification banner — no layout shift */}
        <div className={`message-wrapper${message.text ? '' : ' empty'}`}>
          {message.text && (
            <p
              id="message"
              role="status"
              aria-live="polite"
              className={message.isError ? 'error' : 'ok'}
            >
              <span>{message.text}</span>
              <button
                type="button"
                className="message-dismiss"
                onClick={() => setMessage({ text: '', isError: false })}
                aria-label="Dismiss notification"
              >
                &times;
              </button>
            </p>
          )}
        </div>

        {!token || !user ? (
          <AuthLogin
            onLoginSuccess={handleLoginSuccess}
            setMessage={setMessage}
          />
        ) : (
          <div id="app-view">
            <nav id="tabs" role="navigation" aria-label="Hospital modules">
              {canAccessTab('appointments') && (
                <button
                  type="button"
                  data-tab="appointments"
                  className={activeTab === 'appointments' ? 'active' : ''}
                  onClick={() => setActiveTab('appointments')}
                  aria-selected={activeTab === 'appointments'}
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
                  aria-selected={activeTab === 'icu'}
                >
                  ICU beds &amp; Waitlist
                </button>
              )}
              {canAccessTab('dashboard') && (
                <button
                  type="button"
                  data-tab="dashboard"
                  className={activeTab === 'dashboard' ? 'active' : ''}
                  onClick={() => setActiveTab('dashboard')}
                  aria-selected={activeTab === 'dashboard'}
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
