-- Medislot: Smart Hospital Management System
-- Seed Data Initialization Script

BEGIN;

-- 1. Departments
INSERT INTO departments (id, name, location) VALUES
    ('DEP001', 'Cardiology', 'Building A, Floor 3'),
    ('DEP002', 'Neurology', 'Building B, Floor 2'),
    ('DEP003', 'Orthopedics', 'Building A, Floor 1'),
    ('DEP004', 'General Medicine', 'Building C, Floor 1')
ON CONFLICT (id) DO NOTHING;

-- 2. Doctors
INSERT INTO doctors (id, name, department_id, specialization, email) VALUES
    ('DOC001', 'Dr. Rajesh Sharma', 'DEP001', 'Interventional Cardiologist', 'r.sharma@medislot.com'),
    ('DOC002', 'Dr. Ananya Verma', 'DEP002', 'Neurosurgeon', 'a.verma@medislot.com'),
    ('DOC003', 'Dr. Vikram Patel', 'DEP003', 'Orthopedic Surgeon', 'v.patel@medislot.com'),
    ('DOC004', 'Dr. Sunita Rao', 'DEP004', 'General Physician', 's.rao@medislot.com')
ON CONFLICT (id) DO NOTHING;

-- 3. Operating Theatres & Beds
INSERT INTO rooms (id, room_number, type, status) VALUES
    ('RM101', 'OT-1', 'Operation Theatre', 'AVAILABLE'),
    ('RM102', 'OT-2', 'Operation Theatre', 'AVAILABLE'),
    ('RM201', 'ICU-101', 'ICU Bed', 'AVAILABLE'),
    ('RM202', 'ICU-102', 'ICU Bed', 'AVAILABLE'),
    ('RM301', 'GEN-301', 'General Ward Bed', 'AVAILABLE')
ON CONFLICT (id) DO NOTHING;

-- 4. Bounded Medical Equipment
INSERT INTO equipment (id, name, total_quantity, available_quantity) VALUES
    ('EQ001', 'Ventilator', 5, 5),
    ('EQ002', 'Anesthesia Machine', 3, 3),
    ('EQ003', 'ECG Monitor', 10, 10),
    ('EQ004', 'Dialysis Machine', 2, 2)
ON CONFLICT (id) DO NOTHING;

-- 5. Sample Patients
INSERT INTO patients (id, name, age, gender, contact, email) VALUES
    ('PAT001', 'Amit Kumar', 45, 'Male', '+919876543210', 'amit.k@gmail.com'),
    ('PAT002', 'Priya Singh', 32, 'Female', '+919876543211', 'priya.s@gmail.com'),
    ('PAT003', 'Ramesh Gupta', 58, 'Male', '+919876543212', 'ramesh.g@gmail.com')
ON CONFLICT (id) DO NOTHING;

-- 6. Initial Appointments
INSERT INTO appointments (id, patient_id, doctor_id, appointment_date, time_slot, status, is_emergency) VALUES
    ('APP001', 'PAT001', 'DOC001', CURRENT_DATE, '10:00 AM - 10:30 AM', 'CONFIRMED', FALSE),
    ('APP002', 'PAT002', 'DOC002', CURRENT_DATE, '11:00 AM - 11:30 AM', 'CONFIRMED', FALSE)
ON CONFLICT (id) DO NOTHING;

COMMIT;
