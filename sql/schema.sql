-- ==============================================================================
-- Medislot: Database Schema (Module 1: Database Schema & Integrity)
-- Relational schema with constraints, partial unique indexes, triggers, and views.
-- ==============================================================================

SET client_min_messages = warning;           -- hide the harmless "drop cascades" notices
DROP SCHEMA IF EXISTS medislot CASCADE;
CREATE SCHEMA medislot;
SET search_path = medislot, public, extensions;

CREATE EXTENSION IF NOT EXISTS pgcrypto;      -- provides crypt() = bcrypt password hashing

-- ---------- Master Data ----------
CREATE TABLE doctors (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    specialty TEXT NOT NULL
);

CREATE TABLE patients (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    phone TEXT
);

CREATE TABLE rooms (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    kind TEXT NOT NULL CHECK (kind IN ('consultation', 'operation'))
);

-- ---------- Login Accounts (Module 2) ----------
CREATE TABLE users (
    id            SERIAL PRIMARY KEY,
    username      TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,              -- bcrypt hash, never plain text
    role          TEXT NOT NULL CHECK (role IN ('admin', 'reception', 'doctor', 'patient')),
    doctor_id     INT REFERENCES doctors(id), -- set when role = doctor
    patient_id    INT REFERENCES patients(id) -- set when role = patient
);

-- ---------- Appointments: One 30-minute slot = doctor + start time ----------
CREATE TABLE appointments (
    id         SERIAL PRIMARY KEY,
    patient_id INT NOT NULL REFERENCES patients(id),
    doctor_id  INT NOT NULL REFERENCES doctors(id),
    room_id    INT REFERENCES rooms(id),      -- optional
    start_time TIMESTAMP NOT NULL,
    status     TEXT NOT NULL DEFAULT 'booked' CHECK (status IN ('booked', 'cancelled')),
    created_at TIMESTAMP NOT NULL DEFAULT localtimestamp
);

-- OS/DBMS Concept: Partial Unique Indexes prevent double-booking at the database level.
-- Only active ('booked') slots are constrained, freeing up cancelled slots automatically.
CREATE UNIQUE INDEX one_doctor_per_slot  ON appointments (doctor_id,  start_time) WHERE status = 'booked';
CREATE UNIQUE INDEX one_patient_per_slot ON appointments (patient_id, start_time) WHERE status = 'booked';
CREATE UNIQUE INDEX one_room_per_slot    ON appointments (room_id,    start_time) WHERE status = 'booked' AND room_id IS NOT NULL;

-- DBMS Concept: Trigger to reject bookings in the past
CREATE OR REPLACE FUNCTION reject_past_booking() RETURNS trigger AS $$
BEGIN
    IF NEW.start_time < localtimestamp THEN
        RAISE EXCEPTION 'cannot book a time in the past' USING ERRCODE = 'P0001';
    END IF;
    RETURN NEW;
END $$ LANGUAGE plpgsql;

CREATE TRIGGER no_past_booking BEFORE INSERT ON appointments
    FOR EACH ROW EXECUTE FUNCTION reject_past_booking();

-- DBMS Concept: View joining relational entities for simplified querying
CREATE OR REPLACE VIEW appointment_details AS
    SELECT a.id, a.start_time, a.status, a.patient_id, p.name AS patient,
           a.doctor_id, d.name AS doctor, r.name AS room
    FROM appointments a
    JOIN patients p ON p.id = a.patient_id
    JOIN doctors  d ON d.id = a.doctor_id
    LEFT JOIN rooms r ON r.id = a.room_id;

-- ---------- Waitlist (Module 5): Priority Scheduling Queue ----------
CREATE TABLE waitlist (
    id         SERIAL PRIMARY KEY,
    patient_id INT NOT NULL REFERENCES patients(id),
    doctor_id  INT NOT NULL REFERENCES doctors(id),
    start_time TIMESTAMP NOT NULL,
    priority   TEXT NOT NULL DEFAULT 'normal' CHECK (priority IN ('normal', 'emergency')),
    created_at TIMESTAMP NOT NULL DEFAULT localtimestamp,
    UNIQUE (patient_id, doctor_id, start_time)
);

-- ---------- ICU Beds (Modules 3 + 5): Limited Resource with Queue ----------
CREATE TABLE icu_stays (
    id            SERIAL PRIMARY KEY,
    patient_id    INT NOT NULL REFERENCES patients(id),
    priority      TEXT NOT NULL DEFAULT 'normal' CHECK (priority IN ('normal', 'emergency')),
    status        TEXT NOT NULL DEFAULT 'waiting' CHECK (status IN ('waiting', 'admitted', 'discharged')),
    requested_at  TIMESTAMP NOT NULL DEFAULT localtimestamp,
    admitted_at   TIMESTAMP,
    discharged_at TIMESTAMP
);

-- A patient can be waiting or admitted only once at a time
CREATE UNIQUE INDEX one_active_icu_stay ON icu_stays (patient_id) WHERE status IN ('waiting', 'admitted');

-- ---------- Audit Log (Module 6): Immutable Security Ledger ----------
CREATE TABLE audit_log (
    id       SERIAL PRIMARY KEY,
    at       TIMESTAMP NOT NULL DEFAULT localtimestamp,
    username TEXT,
    action   TEXT NOT NULL,
    detail   TEXT
);
