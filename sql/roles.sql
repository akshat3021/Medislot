


CREATE ROLE hospital_admin;
CREATE ROLE hospital_doctor;
CREATE ROLE hospital_reception;
CREATE ROLE hospital_patient;

GRANT ALL ON ALL TABLES IN SCHEMA public TO hospital_admin;


GRANT SELECT, UPDATE ON Appointments TO hospital_doctor;
GRANT SELECT ON Patients, Doctors, Rooms, Equipment TO hospital_doctor;


GRANT SELECT, INSERT, UPDATE ON Patients, Appointments, Waitlist TO hospital_reception;
GRANT SELECT ON Doctors, Rooms, Equipment TO hospital_reception;


GRANT SELECT, INSERT ON Appointments TO hospital_patient;
GRANT SELECT ON Patients, Doctors TO hospital_patient;


ALTER TABLE Appointments ENABLE ROW LEVEL SECURITY;
ALTER TABLE Patients ENABLE ROW LEVEL SECURITY;


CREATE POLICY patient_view_own_appointments 
ON Appointments 
FOR SELECT 
USING (auth.uid() = patient_id);

CREATE POLICY patient_view_own_data 
ON Patients 
FOR SELECT 
USING (auth.uid() = patient_id);
