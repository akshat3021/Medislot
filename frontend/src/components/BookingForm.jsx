import React, { useState, useEffect, useRef } from 'react';
import { api } from '../api';

const STAFF = ['admin', 'reception'];

export default function BookingForm({ user, setMessage }) {
  const [doctors, setDoctors] = useState([]);
  const [rooms, setRooms] = useState([]);
  const [patients, setPatients] = useState([]);
  const [appointments, setAppointments] = useState([]);

  const [patientId, setPatientId] = useState('');
  const [doctorId, setDoctorId] = useState('');
  const [roomId, setRoomId] = useState('');
  const [startTime, setStartTime] = useState('');
  const [submitting, setSubmitting] = useState(false);
  const [cancellingId, setCancellingId] = useState(null);

  const isStaff = STAFF.includes(user.role);
  const isMountedRef = useRef(true);

  const loadAppointments = async () => {
    try {
      const rows = await api('/appointments');
      if (isMountedRef.current) setAppointments(rows);
    } catch (err) {
      if (isMountedRef.current) setMessage({ text: err.message, isError: true });
    }
  };

  useEffect(() => {
    isMountedRef.current = true;
    const fetchResources = async () => {
      try {
        const [docsData, roomsData] = await Promise.all([
          api('/doctors'),
          api('/rooms'),
        ]);
        if (!isMountedRef.current) return;
        setDoctors(docsData);
        setRooms(roomsData);
        if (docsData.length > 0) setDoctorId(docsData[0].id);

        if (isStaff) {
          const patientsData = await api('/patients');
          if (!isMountedRef.current) return;
          setPatients(patientsData);
          if (patientsData.length > 0) setPatientId(patientsData[0].id);
        }
      } catch (err) {
        if (isMountedRef.current) setMessage({ text: err.message, isError: true });
      }
    };
    fetchResources();
    loadAppointments();
    return () => { isMountedRef.current = false; };
  }, [user.role, isStaff]);

  const getPayload = () => {
    const payload = {
      doctor_id: Number(doctorId),
      room_id: roomId ? Number(roomId) : 0,
      start: startTime.replace('T', ' '),
    };
    if (isStaff && patientId) payload.patient_id = Number(patientId);
    return payload;
  };

  const handleBook = async (e) => {
    e.preventDefault();
    if (!doctorId || !startTime) {
      setMessage({ text: 'Please select a doctor and appointment date/time.', isError: true });
      return;
    }
    setSubmitting(true);
    try {
      await api('/appointments', 'POST', getPayload());
      setMessage({ text: 'Appointment booked successfully.', isError: false });
      loadAppointments();
    } catch (err) {
      setMessage({ text: err.message, isError: true });
    } finally {
      setSubmitting(false);
    }
  };

  const handleWaitlist = async () => {
    if (!doctorId || !startTime) {
      setMessage({ text: 'Please select a doctor and appointment date/time before joining waitlist.', isError: true });
      return;
    }
    setSubmitting(true);
    try {
      await api('/waitlist', 'POST', getPayload());
      setMessage({
        text: 'You are on the waitlist: the slot is booked for you as soon as it is free.',
        isError: false,
      });
      loadAppointments();
    } catch (err) {
      setMessage({ text: err.message, isError: true });
    } finally {
      setSubmitting(false);
    }
  };

  const handleCancel = async (id) => {
    setCancellingId(id);
    try {
      await api(`/appointments/${id}/cancel`, 'POST');
      if (isMountedRef.current) setMessage({ text: 'Appointment cancelled.', isError: false });
      loadAppointments();
    } catch (err) {
      if (isMountedRef.current) setMessage({ text: err.message, isError: true });
    } finally {
      if (isMountedRef.current) setCancellingId(null);
    }
  };

  return (
    <section className="tab" id="tab-appointments">
      {/* ── Booking form (hidden for doctor role) ── */}
      {user.role !== 'doctor' && (
        <form id="book-form" className="can-book" onSubmit={handleBook}>
          <h2>Book an Appointment</h2>

          <div className="booking-form-grid">
            {/* Staff: patient selector spans full width above the two columns */}
            {isStaff && (
              <div className="booking-patient-row">
                <label htmlFor="book-patient" className="staff-only">
                  <span className="label-text">Patient</span>
                  <select
                    id="book-patient"
                    value={patientId}
                    onChange={(e) => setPatientId(e.target.value)}
                  >
                    {patients.map((p) => (
                      <option key={p.id} value={p.id}>{p.name}</option>
                    ))}
                  </select>
                </label>
              </div>
            )}

            {/* Left column: Doctor */}
            <label htmlFor="book-doctor">
              <span className="label-text">Doctor</span>
              <select
                id="book-doctor"
                required
                value={doctorId}
                onChange={(e) => setDoctorId(e.target.value)}
              >
                {doctors.map((d) => (
                  <option key={d.id} value={d.id}>{d.name} — {d.specialty}</option>
                ))}
              </select>
            </label>

            {/* Right column: Date & time */}
            <label htmlFor="book-start">
              <span className="label-text">Date &amp; Time</span>
              <input
                id="book-start"
                type="datetime-local"
                step="1800"
                required
                value={startTime}
                onChange={(e) => setStartTime(e.target.value)}
              />
            </label>

            {/* Left column: Room */}
            <label htmlFor="book-room">
              <span className="label-text">Room</span>
              <span className="label-hint">Optional</span>
              <select
                id="book-room"
                value={roomId}
                onChange={(e) => setRoomId(e.target.value)}
              >
                <option value="">(none)</option>
                {rooms.map((r) => (
                  <option key={r.id} value={r.id}>{r.name}</option>
                ))}
              </select>
            </label>
          </div>

          <div className="form-actions">
            <button type="submit" disabled={submitting}>
              {submitting ? 'Processing…' : 'Book Appointment'}
            </button>
            <button
              type="button"
              id="waitlist-button"
              className="secondary"
              onClick={handleWaitlist}
              disabled={submitting}
            >
              Join Waitlist for This Slot
            </button>
          </div>
        </form>
      )}

      {/* ── Appointments table ── */}
      <h2 style={{ marginTop: user.role !== 'doctor' ? 'var(--space-6)' : 0 }}>
        Scheduled Appointments
      </h2>
      <div className="table-wrapper">
        <table id="appointments-table" aria-label="Scheduled Appointments">
          <thead>
            <tr>
              <th scope="col">When</th>
              <th scope="col">Patient</th>
              <th scope="col">Doctor</th>
              <th scope="col">Room</th>
              <th scope="col">Status</th>
              <th scope="col">Action</th>
            </tr>
          </thead>
          <tbody>
            {appointments.length === 0 ? (
              <tr>
                <td colSpan="6" className="td-empty">No appointments found.</td>
              </tr>
            ) : (
              appointments.map((row) => (
                <tr key={row.id}>
                  <td className="tabular" style={{ fontWeight: 500 }}>{row.start}</td>
                  <td>{row.patient}</td>
                  <td>{row.doctor}</td>
                  <td style={{ color: row.room ? 'inherit' : 'var(--text-muted)' }}>
                    {row.room || '—'}
                  </td>
                  <td>
                    <span className={`badge ${row.status === 'booked' ? 'badge-success' : 'badge-danger'}`}>
                      {row.status}
                    </span>
                  </td>
                  <td>
                    {row.status === 'booked' && (
                      <button
                        type="button"
                        className="danger btn-sm"
                        disabled={cancellingId === row.id}
                        onClick={() => handleCancel(row.id)}
                      >
                        {cancellingId === row.id ? 'Cancelling…' : 'Cancel'}
                      </button>
                    )}
                  </td>
                </tr>
              ))
            )}
          </tbody>
        </table>
      </div>
    </section>
  );
}
