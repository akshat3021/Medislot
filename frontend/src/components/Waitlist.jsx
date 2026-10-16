import React, { useState, useEffect } from 'react';
import { api } from '../api';

export default function Waitlist({ user, setMessage }) {
  const [bedsFree, setBedsFree] = useState(0);
  const [bedsTotal, setBedsTotal] = useState(0);
  const [stays, setStays] = useState([]);
  const [patients, setPatients] = useState([]);
  const [patientId, setPatientId] = useState('');
  const [priority, setPriority] = useState('normal');
  const [submitting, setSubmitting] = useState(false);

  const loadIcu = async () => {
    try {
      const data = await api('/icu');
      setBedsFree(data.beds_free);
      setBedsTotal(data.beds_total);
      setStays(data.stays || []);
    } catch (err) {
      setMessage({ text: err.message, isError: true });
    }
  };

  useEffect(() => {
    loadIcu();
    const interval = setInterval(loadIcu, 3000);

    const loadPatients = async () => {
      try {
        const pList = await api('/patients');
        setPatients(pList);
        if (pList.length > 0) setPatientId(pList[0].id);
      } catch (err) {
        // Patients endpoint might only be accessible to staff
      }
    };
    loadPatients();

    return () => clearInterval(interval);
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
      setTimeout(loadIcu, 500);
    } catch (err) {
      setMessage({ text: err.message, isError: true });
    } finally {
      setSubmitting(false);
    }
  };

  const handleDischarge = async (pId) => {
    try {
      await api('/icu/discharge', 'POST', { patient_id: Number(pId) });
      setMessage({ text: 'Patient discharged successfully.', isError: false });
      loadIcu();
    } catch (err) {
      setMessage({ text: err.message, isError: true });
    }
  };

  return (
    <section className="tab" id="tab-icu">
      <h2>
        ICU Beds & Emergency Priority Queue:{' '}
        <span id="beds" style={{ color: bedsFree > 0 ? '#166534' : '#b91c1c' }}>
          {bedsFree} free of {bedsTotal}
        </span>
      </h2>

      <form id="icu-form" onSubmit={handleRequestBed}>
        <label>
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

        <label>
          Priority
          <select
            id="icu-priority"
            value={priority}
            onChange={(e) => setPriority(e.target.value)}
          >
            <option value="normal">Normal</option>
            <option value="emergency">Emergency (High Priority)</option>
          </select>
        </label>

        <button type="submit" disabled={submitting}>
          {submitting ? 'Requesting...' : 'Request a Bed'}
        </button>
      </form>

      <h2>Current Bed Allocations & Priority Queue</h2>
      <table id="icu-table">
        <thead>
          <tr>
            <th>Patient</th>
            <th>Priority</th>
            <th>Status</th>
            <th>Action</th>
          </tr>
        </thead>
        <tbody>
          {stays.length === 0 ? (
            <tr>
              <td colSpan="4" style={{ textAlign: 'center', color: '#6b7280' }}>
                No active ICU stays or waitlist requests.
              </td>
            </tr>
          ) : (
            stays.map((row, idx) => (
              <tr key={row.patient_id || idx}>
                <td>{row.patient}</td>
                <td>
                  <span
                    style={{
                      padding: '2px 8px',
                      borderRadius: '4px',
                      fontSize: '0.8rem',
                      background: row.priority === 'emergency' ? '#fee2e2' : '#f3f4f6',
                      color: row.priority === 'emergency' ? '#b91c1c' : '#374151',
                      fontWeight: row.priority === 'emergency' ? 'bold' : 'normal',
                    }}
                  >
                    {row.priority}
                  </span>
                </td>
                <td>
                  <span
                    style={{
                      padding: '2px 8px',
                      borderRadius: '4px',
                      fontSize: '0.8rem',
                      background: row.status === 'admitted' ? '#dcfce7' : '#fef3c7',
                      color: row.status === 'admitted' ? '#166534' : '#92400e',
                    }}
                  >
                    {row.status}
                  </span>
                </td>
                <td>
                  {row.status === 'admitted' && (
                    <button
                      className="danger"
                      onClick={() => handleDischarge(row.patient_id)}
                    >
                      Discharge
                    </button>
                  )}
                </td>
              </tr>
            ))
          )}
        </tbody>
      </table>
    </section>
  );
}
