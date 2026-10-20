import React, { useState, useEffect, useRef } from 'react';
import { api } from '../api';

export default function Dashboard({ user, setMessage }) {
  const [totals, setTotals] = useState({ booked: 0, cancelled: 0 });
  const [perDoctor, setPerDoctor] = useState([]);
  const [auditLogs, setAuditLogs] = useState([]);
  const [loading, setLoading] = useState(true);

  const isMountedRef = useRef(true);

  const loadDashboard = async () => {
    setLoading(true);
    try {
      const stats = await api('/stats');
      if (!isMountedRef.current) return;
      setTotals(stats.totals || { booked: 0, cancelled: 0 });
      setPerDoctor(stats.per_doctor || []);

      if (user.role === 'admin') {
        const audit = await api('/audit');
        if (!isMountedRef.current) return;
        setAuditLogs(audit || []);
      }
    } catch (err) {
      if (isMountedRef.current) setMessage({ text: err.message, isError: true });
    } finally {
      if (isMountedRef.current) setLoading(false);
    }
  };

  useEffect(() => {
    isMountedRef.current = true;
    loadDashboard();
    return () => {
      isMountedRef.current = false;
    };
  }, [user.role]);

  return (
    <section className="tab" id="tab-dashboard">
      <h2>Analytics & Resource Utilization Overview</h2>

      {/* Metrics Cards Grid */}
      <div className="dashboard-grid">
        <div className="stat-card" style={{ borderLeft: '4px solid var(--success)' }}>
          <div className="stat-title">Total Booked Appointments</div>
          <div className="stat-value" style={{ color: 'var(--success-text)' }}>
            {totals.booked}
          </div>
        </div>

        <div className="stat-card" style={{ borderLeft: '4px solid var(--danger)' }}>
          <div className="stat-title">Total Cancelled Appointments</div>
          <div className="stat-value" style={{ color: 'var(--danger-text)' }}>
            {totals.cancelled}
          </div>
        </div>

        <div className="stat-card" style={{ borderLeft: '4px solid var(--primary)' }}>
          <div className="stat-title">Active Doctors Monitored</div>
          <div className="stat-value" style={{ color: 'var(--primary)' }}>
            {perDoctor.length}
          </div>
        </div>
      </div>

      <h2>Appointments per Doctor</h2>
      <div className="table-wrapper">
        <table id="stats-table" aria-label="Appointments per Doctor">
          <thead>
            <tr>
              <th scope="col">Doctor</th>
              <th scope="col">Booked Appointments</th>
            </tr>
          </thead>
          <tbody>
            {perDoctor.length === 0 ? (
              <tr>
                <td colSpan="2" style={{ textAlign: 'center', color: 'var(--text-muted)', padding: '1.5rem 1rem' }}>
                  {loading ? 'Loading doctor metrics...' : 'No doctor appointment data recorded.'}
                </td>
              </tr>
            ) : (
              perDoctor.map((d, idx) => (
                <tr key={idx}>
                  <td style={{ fontWeight: 600 }}>{d.doctor}</td>
                  <td style={{ fontVariantNumeric: 'tabular-nums', fontWeight: 500 }}>{d.booked}</td>
                </tr>
              ))
            )}
          </tbody>
        </table>
      </div>

      {user.role === 'admin' && (
        <div id="audit-box" style={{ marginTop: '2rem' }}>
          <h2>Audit Log & Concurrency Thread Tracking (Last 50 Events)</h2>
          <div className="table-wrapper">
            <table id="audit-table" aria-label="Audit Log and Thread Events">
              <thead>
                <tr>
                  <th scope="col">Timestamp</th>
                  <th scope="col">User</th>
                  <th scope="col">Action</th>
                  <th scope="col">Details</th>
                </tr>
              </thead>
              <tbody>
                {auditLogs.length === 0 ? (
                  <tr>
                    <td colSpan="4" style={{ textAlign: 'center', color: 'var(--text-muted)', padding: '1.5rem 1rem' }}>
                      {loading ? 'Loading audit records...' : 'No audit records available.'}
                    </td>
                  </tr>
                ) : (
                  auditLogs.map((log, idx) => (
                    <tr key={idx}>
                      <td style={{ fontSize: '0.8rem', color: 'var(--text-muted)', fontVariantNumeric: 'tabular-nums', whiteSpace: 'nowrap' }}>
                        {log.at}
                      </td>
                      <td>
                        <span style={{ fontWeight: 600 }}>{log.username}</span>
                      </td>
                      <td>
                        <span
                          className="badge badge-info"
                          style={{ fontFamily: 'var(--font-mono)', fontSize: '0.75rem' }}
                        >
                          {log.action}
                        </span>
                      </td>
                      <td style={{ fontSize: '0.85rem', wordBreak: 'break-word' }}>
                        {log.detail}
                      </td>
                    </tr>
                  ))
                )}
              </tbody>
            </table>
          </div>
        </div>
      )}
    </section>
  );
}
