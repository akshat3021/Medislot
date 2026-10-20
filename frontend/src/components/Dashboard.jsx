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
    return () => { isMountedRef.current = false; };
  }, [user.role]);

  // Derive cancellation rate only when data is available
  const totalAppointments = totals.booked + totals.cancelled;
  const cancelRate = totalAppointments > 0
    ? Math.round((totals.cancelled / totalAppointments) * 100)
    : null;

  return (
    <section className="tab" id="tab-dashboard">
      <h2>Operations Overview</h2>

      {/* ── Metric cards ── */}
      <div className="dashboard-grid">
        <div
          className="stat-card"
          style={{ '--stat-accent': 'var(--success)' }}
          aria-label={`${totals.booked} booked appointments`}
        >
          <div className="stat-title">Booked</div>
          <div className="stat-value" style={{ color: 'var(--success-text)' }}>
            {loading ? '—' : totals.booked}
          </div>
        </div>

        <div
          className="stat-card"
          style={{ '--stat-accent': 'var(--danger)' }}
          aria-label={`${totals.cancelled} cancelled appointments`}
        >
          <div className="stat-title">Cancelled</div>
          <div className="stat-value" style={{ color: 'var(--danger-text)' }}>
            {loading ? '—' : totals.cancelled}
          </div>
        </div>

        <div
          className="stat-card"
          style={{ '--stat-accent': 'var(--primary)' }}
          aria-label={`${perDoctor.length} doctors active`}
        >
          <div className="stat-title">Doctors Active</div>
          <div className="stat-value" style={{ color: 'var(--primary)' }}>
            {loading ? '—' : perDoctor.length}
          </div>
        </div>

        {cancelRate !== null && !loading && (
          <div
            className="stat-card"
            style={{ '--stat-accent': 'var(--warning)' }}
            aria-label={`${cancelRate}% cancellation rate`}
          >
            <div className="stat-title">Cancellation Rate</div>
            <div className="stat-value" style={{ color: 'var(--warning-text)' }}>
              {cancelRate}%
            </div>
          </div>
        )}
      </div>

      {/* ── Per-doctor table ── */}
      <h2 style={{ marginTop: 'var(--space-2)' }}>Appointments by Doctor</h2>
      <div className="table-wrapper">
        <table id="stats-table" aria-label="Appointments per Doctor">
          <thead>
            <tr>
              <th scope="col">Doctor</th>
              <th scope="col">Booked</th>
            </tr>
          </thead>
          <tbody>
            {perDoctor.length === 0 ? (
              <tr>
                <td colSpan="2" className="td-empty">
                  {loading ? 'Loading…' : 'No data recorded.'}
                </td>
              </tr>
            ) : (
              perDoctor.map((d, idx) => (
                <tr key={idx}>
                  <td style={{ fontWeight: 600 }}>{d.doctor}</td>
                  <td className="tabular">{d.booked}</td>
                </tr>
              ))
            )}
          </tbody>
        </table>
      </div>

      {/* ── Audit log (admin only) ── */}
      {user.role === 'admin' && (
        <div id="audit-box" style={{ marginTop: 'var(--space-8)' }}>
          <h2>Audit Log — Last 50 Events</h2>
          <div className="table-wrapper">
            <table id="audit-table" aria-label="System audit log">
              <thead>
                <tr>
                  <th scope="col">Time</th>
                  <th scope="col">User</th>
                  <th scope="col">Action</th>
                  <th scope="col">Details</th>
                </tr>
              </thead>
              <tbody>
                {auditLogs.length === 0 ? (
                  <tr>
                    <td colSpan="4" className="td-empty">
                      {loading ? 'Loading audit records…' : 'No records available.'}
                    </td>
                  </tr>
                ) : (
                  auditLogs.map((log, idx) => (
                    <tr key={idx}>
                      <td
                        className="tabular"
                        style={{
                          fontSize: '0.8rem',
                          color: 'var(--text-muted)',
                          whiteSpace: 'nowrap',
                        }}
                      >
                        {log.at}
                      </td>
                      <td style={{ fontWeight: 600 }}>{log.username}</td>
                      <td>
                        <span
                          className="badge badge-info text-mono"
                          style={{ fontSize: '0.7rem' }}
                        >
                          {log.action}
                        </span>
                      </td>
                      <td
                        style={{
                          fontSize: '0.85rem',
                          wordBreak: 'break-word',
                          color: 'var(--text-secondary)',
                        }}
                      >
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
