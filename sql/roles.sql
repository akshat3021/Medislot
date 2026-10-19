-- ==============================================================================
-- Medislot: Roles, Permissions, & Row-Level Security (RLS) (Module 2 Demo)
-- College DBMS Concept: Discretionary Access Control (DAC) via GRANT/REVOKE
-- and fine-grained Row-Level Security (RLS).
-- ==============================================================================

SET search_path = medislot, public, extensions;

-- 1. Create Roles (NOLOGIN groups)
DO $$
BEGIN
    IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'medislot_reception') THEN
        CREATE ROLE medislot_reception NOLOGIN;
    END IF;
    IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'medislot_patient') THEN
        CREATE ROLE medislot_patient NOLOGIN;
    END IF;
END $$;

-- 2. Schema Usage Permissions
GRANT USAGE ON SCHEMA medislot TO medislot_reception, medislot_patient;

-- 3. Reception Permissions: Can read/write appointments, patients, rooms, waitlist, ICU
GRANT SELECT, INSERT, UPDATE ON doctors, patients, rooms, appointments, waitlist, icu_stays TO medislot_reception;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA medislot TO medislot_reception;

-- 4. Audit Log Immutability: Even staff roles CANNOT UPDATE or DELETE audit records!
REVOKE UPDATE, DELETE, TRUNCATE ON audit_log FROM PUBLIC, medislot_reception, medislot_patient;
GRANT INSERT, SELECT ON audit_log TO medislot_reception;

-- 5. Patient Permissions: Read reference tables, select/insert waitlist
GRANT SELECT ON doctors, rooms, appointment_details TO medislot_patient;
GRANT SELECT, INSERT, UPDATE ON appointments TO medislot_patient;
GRANT SELECT, INSERT ON waitlist TO medislot_patient;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA medislot TO medislot_patient;

-- 6. Row-Level Security (RLS) Demonstration on Appointments
ALTER TABLE appointments ENABLE ROW LEVEL SECURITY;

-- Drop policy if exists to make script re-runnable
DROP POLICY IF EXISTS patient_own_appointments ON appointments;

-- Policy: Patients can only see and manage rows where patient_id matches their session variable
CREATE POLICY patient_own_appointments ON appointments
    FOR ALL
    TO medislot_patient
    USING (patient_id = NULLIF(current_setting('app.patient_id', true), '')::INT)
    WITH CHECK (patient_id = NULLIF(current_setting('app.patient_id', true), '')::INT);

-- ==============================================================================
-- HOW TO TEST THIS IN PSQL / SUPABASE SQL EDITOR:
-- ==============================================================================
-- 1. Set the role to patient:
--    SET ROLE medislot_patient;
-- 2. Set the patient session context:
--    SET app.patient_id = '1';
-- 3. Query appointments (only patient 1's appointments will be returned):
--    SELECT * FROM medislot.appointments;
-- 4. Try to delete or update audit_log (will be rejected with permission denied):
--    DELETE FROM medislot.audit_log;
-- 5. Reset back to postgres / admin role:
--    RESET ROLE;
