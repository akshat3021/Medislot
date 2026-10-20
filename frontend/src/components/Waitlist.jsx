import React, { useState, useEffect, useRef } from 'react';
import { api } from '../api';

export default function Waitlist({ user, setMessage }) {
  const [bedsFree, setBedsFree] = useState(0);
  const [bedsTotal, setBedsTotal] = useState(0);
  const [stays, setStays] = useState([]);
  const [patients, setPatients] = useState([]);
  const [patientId, setPatientId] = useState('');
  const [priority, setPriority] = useState('normal');
  const [submitting, setSubmitting] = useState(false);
  const [dischargingId, setDischargingId] = useState(null);

  const isFetchingRef = useRef(false);
  const isMountedRef = useRef(true);

  const loadIcu = async () => {
    if (isFetchingRef.current) return;
    if (document.visibilityState === 'hidden') return;

    isFetchingRef.current = true;
    try {
      const data = await api('/icu');
      if (!isMountedRef.current) return;
      setBedsFree(data.beds_free ?? 0);
      setBedsTotal(data.beds_total ?? 0);
      setStays(data.stays || []);
    } catch (err) {
      if (isMountedRef.current) setMessage({ text: err.message, isError: true });
    } finally {
      isFetchingRef.current = false;
    }
  };

  useEffect(() => {
    isMountedRef.current = true;
    loadIcu();

    const interval = setInterval(loadIcu, 4000);

    const handleVisibilityChange = () => {
      if (document.visibilityState === 'visible') loadIcu();
    };
    document.addEventListener('visibilitychange', handleVisibilityChange);

    const loadPatients = async () => {
      try {
        const pList = await api('/patients');
        if (!isMountedRef.current) return;
        setPatients(pList);
        if (pList.length > 0) setPatientId(pList[0].id);
      } catch {
        // Patients endpoint may not be accessible to all roles
      }
    };
    loadPatients();

    return () => {
      isMountedRef.current = false;
      clearInterval(interval);
      document.removeEventListener('visibilitychange', handleVisibilityChange);
    };
  }, []);

  const handleRequestBed = async (e) => {
    e.preventDefault();
    if (!patientId) {
      setMessage({ text: 'Please select a patient.', isError: true });
      return;
    }
    setSubmitting(true);
    try {
      await api('/icu/request', 'POST', {
        patient_id: Number(patientId),
        priority,
      });
      setMessage({
        text: 'Request sent. The scheduler admits patients by priority when a bed is free.',
        isError: false,
      });
      setTimeout(loadIcu, 400);
    } catch (err) {
      setMessage({ text: err.message, isError: true });
    } finally {
      setSubmitting(false);
    }
  };

  const handleDischarge = async (pId) => {
    setDischargingId(pId);
    try {
      await api('/icu/discharge', 'POST', { patient_id: Number(pId) });
      if (isMountedRef.current)
        setMessage({ text: 'Patient discharged successfully.', isError: false });
      loadIcu();
    } catch (err) {
      if (isMountedRef.current) setMessage({ text: err.message, isError: true });
    } finally {
      if (isMountedRef.current) setDischargingId(null);
    }
  };

  // Guard against bedsTotal=0 before data loads
  const occupiedBeds = bedsTotal > 0 ? Math.max(0, bedsTotal - bedsFree) : 0;
  const occupancyPct = bedsTotal > 0
    ? Math.min(100, Math.round((occupiedBeds / bedsTotal) * 100))
    : 0;

  const fillColor =
    occupancyPct > 85 ? 'var(--danger)' :
    occupancyPct > 60 ? 'var(--warning)' :
    'var(--success)';

  return (
    <section className="tab" id="tab-icu">
      <h2>ICU Bed Allocation &amp; Emergency Triage</h2>

      {/* ── Capacity monitor ── */}
      <div className="icu-capacity-card" role="region" aria-label="ICU bed capacity">
        <div className="icu-capacity-label">
          <div className="icu-capacity-title">Bed Availability</div>
          <div className="icu-beds-count">
            <span
              id="beds"
              style={{ color: bedsFree > 0 ? 'var(--success-text)' : 'var(--danger-text)' }}
            >
              {bedsFree} free
            </span>
            <span style={{ color: 'var(--text-muted)', fontWeight: 500, fontSize: '1rem' }}>
              {' '}/ {bedsTotal} total
            </span>
          </div>
          {bedsTotal > 0 && (
            <div className="icu-capacity-util">
              {occupancyPct}% occupied
            </div>
          )}
        </div>

        {bedsTotal > 0 && (
          <div
            className="capacity-track"
            role="progressbar"
            aria-valuenow={occupancyPct}
            aria-valuemin={0}
            aria-valuemax={100}
            aria-label={`${occupancyPct}% beds occupied`}
          >
            <div
              className="capacity-fill"
              style={{ width: `${occupancyPct}%`, backgroundColor: fillColor }}
            />
          </div>
        )}
      </div>

      {/* ── Bed request form ── */}
      <form id="icu-form" onSubmit={handleRequestBed}>
        <h3>Request an ICU Bed</h3>
        <div className="booking-form-grid">
          <label htmlFor="icu-patient">
            <span className="label-text">Patient</span>
            <select
              id="icu-patient"
              value={patientId}
              onChange={(e) => setPatientId(e.target.value)}
            >
              {patients.length === 0 && (
                <option value="">Loading patients…</option>
              )}
              {patients.map((p) => (
                <option key={p.id} value={p.id}>{p.name}</option>
              ))}
            </select>
          </label>

          <label htmlFor="icu-priority">
            <span className="label-text">Priority Level</span>
            <select
              id="icu-priority"
              value={priority}
              onChange={(e) => setPriority(e.target.value)}
            >
              <option value="normal">Normal</option>
              <option value="emergency">Emergency — High Priority</option>
            </select>
          </label>
        </div>

        <div className="form-actions">
          <button type="submit" disabled={submitting || patients.length === 0}>
            {submitting ? 'Requesting…' : 'Request Bed'}
          </button>
        </div>
      </form>

      {/* ── Allocation table ── */}
      <h2 style={{ marginTop: 'var(--space-6)' }}>Active Allocations &amp; Priority Queue</h2>
      <div className="table-wrapper">
        <table id="icu-table" aria-label="ICU Bed Allocations and Queue">
          <thead>
            <tr>
              <th scope="col">Patient</th>
              <th scope="col">Priority</th>
              <th scope="col">Status</th>
              <th scope="col">Action</th>
            </tr>
          </thead>
          <tbody>
            {stays.length === 0 ? (
              <tr>
                <td colSpan="4" className="td-empty">
                  No active ICU stays or waitlist requests.
                </td>
              </tr>
            ) : (
              stays.map((row, idx) => {
                const isEmergency = row.priority === 'emergency';
                const isAdmitted  = row.status === 'admitted';
                return (
                  <tr
                    key={row.patient_id || idx}
                    className={isEmergency ? 'row-emergency' : ''}
                  >
                    <td style={{ fontWeight: 600 }}>{row.patient}</td>
                    <td>
                      <span className={`badge ${isEmergency ? 'badge-emergency' : 'badge-info'}`}>
                        {isEmergency ? 'Emergency' : 'Normal'}
                      </span>
                    </td>
                    <td>
                      <span className={`badge ${isAdmitted ? 'badge-success' : 'badge-warning'}`}>
                        {isAdmitted ? 'Admitted' : 'Waiting'}
                      </span>
                    </td>
                    <td>
                      {isAdmitted && (
                        <button
                          type="button"
                          className="danger btn-sm"
                          disabled={dischargingId === row.patient_id}
                          onClick={() => handleDischarge(row.patient_id)}
                        >
                          {dischargingId === row.patient_id ? 'Discharging…' : 'Discharge'}
                        </button>
                      )}
                    </td>
                  </tr>
                );
              })
            )}
          </tbody>
        </table>
      </div>
    </section>
  );
}
