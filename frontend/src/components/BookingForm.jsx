import React, { useState, useEffect } from 'react';
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

  const isStaff = STAFF.includes(user.role);

  // Load appointments
  const loadAppointments = async () => {
    try {
      const rows = await api('/appointments');
      setAppointments(rows);
    } catch (err) {
      setMessage({ text: err.message, isError: true });
    }
  };

  // Initial resource loading
  useEffect(() => {
    const fetchResources = async () => {
      try {
        const [docsData, roomsData] = await Promise.all([
          api('/doctors'),
          api('/rooms'),
        ]);
        setDoctors(docsData);
        setRooms(roomsData);
        if (docsData.length > 0) {
          setDoctorId(docsData[0].id);
        }

        if (isStaff) {
          const patientsData = await api('/patients');
          setPatients(patientsData);
          if (patientsData.length > 0) {
            setPatientId(patientsData[0].id);
          }
        }
      } catch (err) {
        setMessage({ text: err.message, isError: true });
      }
    };

    fetchResources();
    loadAppointments();
  }, [user.role, isStaff]);

  const getPayload = () => {
    const payload = {
      doctor_id: Number(doctorId),
      room_id: roomId ? Number(roomId) : 0,
      start: startTime.replace('T', ' '),
    };
    if (isStaff && patientId) {
      payload.patient_id = Number(patientId);
    }
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
    try {
      await api(`/appointments/${id}/cancel`, 'POST');
      setMessage({ text: 'Appointment cancelled.', isError: false });
      loadAppointments();
    } catch (err) {
      setMessage({ text: err.message, isError: true });
    }
  };

  return (
    <section className="tab" id="tab-appointments">
      {user.role !== 'doctor' && (
        <form id="book-form" className="can-book" onSubmit={handleBook}>
          <h2>Book an Appointment or Procedure</h2>

          {isStaff && (
            <label className="staff-only">
              Patient
              <select
                id="book-patient"
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
          )}

          <label>
            Doctor
            <select
              id="book-doctor"
              required
              value={doctorId}
              onChange={(e) => setDoctorId(e.target.value)}
            >
              {doctors.map((d) => (
                <option key={d.id} value={d.id}>
                  {d.name} - {d.specialty}
                </option>
              ))}
            </select>
          </label>

          <label>
            Room (optional)
            <select
              id="book-room"
              value={roomId}
              onChange={(e) => setRoomId(e.target.value)}
            >
              <option value="">(none)</option>
              {rooms.map((r) => (
                <option key={r.id} value={r.id}>
                  {r.name}
                </option>
              ))}
            </select>
          </label>

          <label>
            Date and time
            <input
              id="book-start"
              type="datetime-local"
              step="1800"
              required
              value={startTime}
              onChange={(e) => setStartTime(e.target.value)}
            />
          </label>

          <div style={{ display: 'flex', gap: '0.75rem', marginTop: '1rem' }}>
            <button type="submit" disabled={submitting}>
              {submitting ? 'Processing...' : 'Book'}
            </button>
            <button
              type="button"
              id="waitlist-button"
              onClick={handleWaitlist}
              disabled={submitting}
              style={{ background: '#4b5563' }}
            >
              Join waitlist for this slot
            </button>
          </div>
        </form>
      )}

      <h2>Scheduled Appointments</h2>
      <table id="appointments-table">
        <thead>
          <tr>
            <th>When</th>
            <th>Patient</th>
            <th>Doctor</th>
            <th>Room</th>
            <th>Status</th>
            <th>Action</th>
          </tr>
        </thead>
        <tbody>
          {appointments.length === 0 ? (
            <tr>
              <td colSpan="6" style={{ textAlign: 'center', color: '#6b7280' }}>
                No appointments found.
              </td>
            </tr>
          ) : (
            appointments.map((row) => (
              <tr key={row.id}>
                <td>{row.start}</td>
                <td>{row.patient}</td>
                <td>{row.doctor}</td>
                <td>{row.room || '-'}</td>
                <td>
                  <span
                    style={{
                      padding: '2px 8px',
                      borderRadius: '4px',
                      fontSize: '0.8rem',
                      background: row.status === 'booked' ? '#dcfce7' : '#fee2e2',
                      color: row.status === 'booked' ? '#166534' : '#991b1b',
                    }}
                  >
                    {row.status}
                  </span>
                </td>
                <td>
                  {row.status === 'booked' && (
                    <button
                      className="danger"
                      onClick={() => handleCancel(row.id)}
                    >
                      Cancel
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
