-- ==============================================================================
-- Medislot: Seed Data
-- Demo records with passwords hashed via PostgreSQL pgcrypto (bcrypt)
-- ==============================================================================

SET search_path = medislot, public, extensions;

INSERT INTO doctors (name, specialty) VALUES
    ('Dr. Meera Mehta', 'Cardiology'),
    ('Dr. Arjun Rao',   'Orthopedics'),
    ('Dr. Kabir Singh', 'General Medicine');

INSERT INTO patients (name, phone) VALUES
    ('Alice Kumar', '9876500001'),
    ('Bob Sharma',  '9876500002'),
    ('Carol Nair',  '9876500003');

INSERT INTO rooms (name, kind) VALUES
    ('Consultation 1', 'consultation'),
    ('Consultation 2', 'consultation'),
    ('Operation Theatre 1', 'operation');

INSERT INTO users (username, password_hash, role, doctor_id, patient_id) VALUES
    ('admin',     crypt('admin123',     gen_salt('bf')), 'admin',     NULL, NULL),
    ('reception', crypt('reception123', gen_salt('bf')), 'reception', NULL, NULL),
    ('drmehta',   crypt('doctor123',    gen_salt('bf')), 'doctor',    1,    NULL),
    ('alice',     crypt('patient123',   gen_salt('bf')), 'patient',   NULL, 1),
    ('bob',       crypt('patient123',   gen_salt('bf')), 'patient',   NULL, 2);
