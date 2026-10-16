import React, { useState, useEffect } from 'react';
import { api } from '../api';

export default function Dashboard({ user, setMessage }) {
  const [totals, setTotals] = useState({ booked: 0, cancelled: 0 });
  const [perDoctor, setPerDoctor] = useState([]);
  const [auditLogs, setAuditLogs] = useState([]);
  const [loading, setLoading] = useState(true);

  const loadDashboard = async () => {
    setLoading(true);
    try {
      const stats = await api('/stats');
      setTotals(stats.totals || { booked: 0, cancelled: 0 });
      setPerDoctor(stats.per_doctor || []);

      if (user.role === 'admin') {
        const audit = await api('/audit');
        setAuditLogs(audit || []);
      }
    } catch (err) {
      setMessage({ text: err.message, isError: true });
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadDashboard();
  }, [user.role]);

  return (
    <section className="tab" id="tab-dashboard">
      <h2>Analytics & Resource Utilization Overview</h2>

      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(200px, 1fr))', gap: '1rem', marginBottom: '1.5rem' }}>
        <div style={{ background: '#f0fdf4', padding: '1rem', borderRadius: '8px', border: '1px solid #bbf7d0' }}>
          <div style={{ fontSize: '0.85rem', color: '#166534', fontWeight: 600 }}>Total Booked</div>
          <div style={{ fontSize: '1.8rem', fontWeight: 700, color: '#15803d' }}>{totals.booked}</div>
        </div>
        <div style={{ background: '#fef2f2', padding: '1rem', borderRadius: '8px', border: '1px solid #fecaca' }}>
          <div style={{ fontSize: '0.85rem', color: '#991b1b', fontWeight: 600 }}>Total Cancelled</div>
          <div style={{ fontSize: '1.8rem', fontWeight: 700, color: '#b91c1c' }}>{totals.cancelled}</div>
        </div>
        <div style={{ background: '#eff6ff', padding: '1rem', borderRadius: '8px', border: '1px solid #bfdbfe' }}>
          <div style={{ fontSize: '0.85rem', color: '#1e40af', fontWeight: 600 }}>Active Doctors Monitored</div>
          <div style={{ fontSize: '1.8rem', fontWeight: 700, color: '#2563eb' }}>{perDoctor.length}</div>
        </div>
      </div>

      <h2>Appointments per Doctor</h2>
      <table id="stats-table">
        <thead>
          <tr>
            <th>Doctor</th>
            <th>Booked Appointments</th>
          </tr>
        </thead>
        <tbody>
          {perDoctor.length === 0 ? (
            <tr>
              <td colSpan="2" style={{ textAlign: 'center', color: '#6b7280' }}>
                No doctor appointment data recorded.
              </td>
            </tr>
          ) : (
            perDoctor.map((d, idx) => (
              <tr key={idx}>
                <td><strong>{d.doctor}</strong></td>
                <td>{d.booked}</td>
              </tr>
            ))
          )}
        </tbody>
      </table>

      {user.role === 'admin' && (
        <div id="audit-box" style={{ marginTop: '2rem' }}>
          <h2>Audit Log & Thread Tracking (Last 50 Events)</h2>
          <table id="audit-table">
            <thead>
              <tr>
                <th>Time</th>
                <th>User</th>
                <th>Action</th>
                <th>Details</th>
              </tr>
            </thead>
            <tbody>
              {auditLogs.length === 0 ? (
                <tr>
                  <td colSpan="4" style={{ textAlign: 'center', color: '#6b7280' }}>
                    No audit records available.
                  </td>
                </tr>
              ) : (
                auditLogs.map((log, idx) => (
                  <tr key={idx}>
                    <td style={{ fontSize: '0.85rem', color: '#4b5563' }}>{log.at}</td>
                    <td><span style={{ fontWeight: 600 }}>{log.username}</span></td>
                    <td>
                      <span
                        style={{
                          padding: '2px 6px',
                          borderRadius: '4px',
                          fontSize: '0.75rem',
                          background: '#e0e7ff',
                          color: '#3730a3',
                          fontFamily: 'monospace',
                        }}
                      >
                        {log.action}
                      </span>
                    </td>
                    <td>{log.detail}</td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      )}
    </section>
  );
}
