-- Medislot: Smart Hospital Management System
-- Database Schema DDL (PostgreSQL)

BEGIN;

-- Cleanup existing tables if re-initializing
DROP TABLE IF EXISTS audit_log CASCADE;
DROP TABLE IF EXISTS waitlist CASCADE;
DROP TABLE IF EXISTS procedure_resources CASCADE;
DROP TABLE IF EXISTS appointments CASCADE;
DROP TABLE IF EXISTS patients CASCADE;
DROP TABLE IF EXISTS equipment CASCADE;
DROP TABLE IF EXISTS rooms CASCADE;
DROP TABLE IF EXISTS doctors CASCADE;
DROP TABLE IF EXISTS departments CASCADE;

-- 1. Departments Table
CREATE TABLE departments (
    id VARCHAR(20) PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    location VARCHAR(150) NOT NULL
);

-- 2. Doctors Table
CREATE TABLE doctors (
    id VARCHAR(20) PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    department_id VARCHAR(20) REFERENCES departments(id) ON DELETE CASCADE,
    specialization VARCHAR(100) NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL
);

-- 3. Operating Theatres & Beds Table
CREATE TABLE rooms (
    id VARCHAR(20) PRIMARY KEY,
    room_number VARCHAR(50) NOT NULL,
    type VARCHAR(50) NOT NULL, -- e.g., 'Operation Theatre', 'ICU Bed', 'General Ward Bed'
    status VARCHAR(20) DEFAULT 'AVAILABLE' CHECK (status IN ('AVAILABLE', 'OCCUPIED', 'MAINTENANCE'))
);

-- 4. Bounded Medical Equipment Table
CREATE TABLE equipment (
    id VARCHAR(20) PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    total_quantity INT NOT NULL CHECK (total_quantity >= 0),
    available_quantity INT NOT NULL CHECK (available_quantity >= 0)
);

-- 5. Patients Table
CREATE TABLE patients (
    id VARCHAR(20) PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    age INT CHECK (age > 0),
    gender VARCHAR(10),
    contact VARCHAR(20),
    email VARCHAR(100) UNIQUE
);

-- 6. Appointments Table
CREATE TABLE appointments (
    id VARCHAR(20) PRIMARY KEY,
    patient_id VARCHAR(20) REFERENCES patients(id) ON DELETE CASCADE,
    doctor_id VARCHAR(20) REFERENCES doctors(id) ON DELETE CASCADE,
    appointment_date DATE NOT NULL,
    time_slot VARCHAR(50) NOT NULL,
    status VARCHAR(20) DEFAULT 'CONFIRMED' CHECK (status IN ('CONFIRMED', 'CANCELLED', 'COMPLETED', 'WAITLISTED')),
    is_emergency BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 7. Multi-Resource Procedure Resources Table (Junction Table for Deadlock Engine)
CREATE TABLE procedure_resources (
    id SERIAL PRIMARY KEY,
    appointment_id VARCHAR(20) REFERENCES appointments(id) ON DELETE CASCADE,
    room_id VARCHAR(20) REFERENCES rooms(id) ON DELETE SET NULL,
    equipment_id VARCHAR(20) REFERENCES equipment(id) ON DELETE SET NULL,
    status VARCHAR(20) DEFAULT 'LOCKED' CHECK (status IN ('LOCKED', 'RELEASED', 'ROLLED_BACK'))
);

-- 8. Priority Waitlist Table
CREATE TABLE waitlist (
    id SERIAL PRIMARY KEY,
    patient_id VARCHAR(20) REFERENCES patients(id) ON DELETE CASCADE,
    doctor_id VARCHAR(20) REFERENCES doctors(id) ON DELETE CASCADE,
    priority INT DEFAULT 1, -- 1 = Routine, 5 = Emergency High Priority
    status VARCHAR(20) DEFAULT 'WAITING' CHECK (status IN ('WAITING', 'PROMOTED', 'EXPIRED')),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 9. Audit Log Table (OS Thread & Concurrency Event Tracking)
CREATE TABLE audit_log (
    id SERIAL PRIMARY KEY,
    user_id VARCHAR(50),
    action VARCHAR(255) NOT NULL,
    table_affected VARCHAR(50),
    process_thread_id VARCHAR(50),
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

COMMIT;
