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

    const interval = setInterval(() => {
      loadIcu();
    }, 4000);

    const handleVisibilityChange = () => {
      if (document.visibilityState === 'visible') {
        loadIcu();
      }
    };

    document.addEventListener('visibilitychange', handleVisibilityChange);

    const loadPatients = async () => {
      try {
        const pList = await api('/patients');
        if (!isMountedRef.current) return;
        setPatients(pList);
        if (pList.length > 0) setPatientId(pList[0].id);
      } catch (err) {
        // Patients endpoint might only be accessible to staff
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
    setSubmitting(true);
    try {
      await api('/icu/request', 'POST', {
        patient_id: Number(patientId),
        priority,
      });
      setMessage({
        text: 'Request sent: the scheduler admits patients by priority when a bed is free.',
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
      if (isMountedRef.current) setMessage({ text: 'Patient discharged successfully.', isError: false });
      loadIcu();
    } catch (err) {
      if (isMountedRef.current) setMessage({ text: err.message, isError: true });
    } finally {
      if (isMountedRef.current) setDischargingId(null);
    }
  };

  const occupiedBeds = Math.max(0, bedsTotal - bedsFree);
  const occupancyPercent = bedsTotal > 0 ? Math.round((occupiedBeds / bedsTotal) * 100) : 0;

  return (
    <section className="tab" id="tab-icu">
      <h2>ICU Bed Allocation & Emergency Triage Queue</h2>

      {/* Visual ICU Capacity Monitor */}
      <div className="icu-capacity-card" role="region" aria-label="ICU capacity meter">
        <div>
          <div style={{ fontSize: '0.8rem', fontWeight: 600, textTransform: 'uppercase', color: 'var(--text-muted)' }}>
            Bed Availability
          </div>
          <div style={{ fontSize: '1.25rem', fontWeight: 700, marginTop: '0.2rem' }}>
            <span
              id="beds"
              style={{
                color: bedsFree > 0 ? 'var(--success-text)' : 'var(--danger-text)',
              }}
            >
              {bedsFree} free of {bedsTotal}
            </span>
            <span style={{ fontSize: '0.85rem', fontWeight: 500, color: 'var(--text-muted)', marginLeft: '0.5rem' }}>
              ({occupancyPercent}% capacity utilized)
            </span>
          </div>
        </div>

        <div className="capacity-track" title={`${occupancyPercent}% beds occupied`}>
          <div
            className="capacity-fill"
            style={{
              width: `${occupancyPercent}%`,
              background: occupancyPercent > 80 ? 'var(--danger)' : occupancyPercent > 50 ? 'var(--warning)' : 'var(--success)',
            }}
          />
        </div>
      </div>

      <form id="icu-form" onSubmit={handleRequestBed}>
        <h3 style={{ margin: '0 0 1rem', fontSize: '1rem', fontWeight: 600 }}>Request an ICU Bed</h3>
        <label htmlFor="icu-patient">
          Patient
          <select
            id="icu-patient"
            value={patientId}
            onChange={(e) => setPatientId(e.target.value)}
          >
            {patients.map((p) => (
              <option key={p.id} value={p.id}>
                {p.name}
              </option>
            ))}
          </select>
        </label>

        <label htmlFor="icu-priority">
          Priority Level
          <select
            id="icu-priority"
            value={priority}
            onChange={(e) => setPriority(e.target.value)}
          >
            <option value="normal">Normal Priority</option>
            <option value="emergency">Emergency (High Priority Immediate)</option>
          </select>
        </label>

        <div style={{ marginTop: '1.25rem' }}>
          <button type="submit" disabled={submitting}>
            {submitting ? 'Requesting Bed...' : 'Request a Bed'}
          </button>
        </div>
      </form>

      <h2>Active Allocations & Priority Queue</h2>
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
                <td colSpan="4" style={{ textAlign: 'center', color: 'var(--text-muted)', padding: '1.5rem 1rem' }}>
                  No active ICU stays or waitlist requests.
                </td>
              </tr>
            ) : (
              stays.map((row, idx) => {
                const isEmergency = row.priority === 'emergency';
                const isAdmitted = row.status === 'admitted';
                return (
                  <tr key={row.patient_id || idx} className={isEmergency ? 'row-emergency' : ''}>
                    <td style={{ fontWeight: 500 }}>{row.patient}</td>
                    <td>
                      <span className={`badge ${isEmergency ? 'badge-emergency' : 'badge-info'}`}>
                        {isEmergency ? 'URGENT: Emergency' : row.priority}
                      </span>
                    </td>
                    <td>
                      <span className={`badge ${isAdmitted ? 'badge-success' : 'badge-warning'}`}>
                        {row.status}
                      </span>
                    </td>
                    <td>
                      {isAdmitted && (
                        <button
                          type="button"
                          className="danger"
                          disabled={dischargingId === row.patient_id}
                          onClick={() => handleDischarge(row.patient_id)}
                          style={{ padding: '0.3rem 0.7rem', fontSize: '0.8rem' }}
                        >
                          {dischargingId === row.patient_id ? 'Discharging...' : 'Discharge'}
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
