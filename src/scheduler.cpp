
#include <iostream>
#include <queue>
#include <string>
#include <vector>
using namespace std;

// Details of a patient
struct Patient {
    int id;
    string name;
    bool emergency;
};

// Decide which patient will be treated first
struct ComparePatient {
    bool operator()(Patient p1, Patient p2) {
        // Emergency patients get higher priority
        if (p1.emergency != p2.emergency) {
            return !p1.emergency;
        }

        // Smaller ID gets priority if both have the same priority
        return p1.id > p2.id;
    }
};

int main() {
    priority_queue<Patient, vector<Patient>, ComparePatient> patients;

    int n;
    cout << "Enter number of patients: ";
    cin >> n;

    // Take patient details from the user
    for (int i = 0; i < n; i++) {
        Patient p;

        cout << "\nEnter details of patient " << i + 1 << ":\n";

        cout << "Patient ID: ";
        cin >> p.id;

        cout << "Patient Name: ";
        cin >> ws;
        getline(cin, p.name);

        int choice;
        cout << "Is it an emergency? (1 = Yes, 0 = No): ";
        cin >> choice;

        p.emergency = (choice == 1);

        patients.push(p);
    }

    cout << "\nMedislot Emergency Priority Scheduler\n\n";

    // Process patients according to priority
    while (!patients.empty()) {
        Patient p = patients.top();
        patients.pop();

        cout << "Patient ID: " << p.id << endl;
        cout << "Patient Name: " << p.name << endl;

        if (p.emergency) {
            cout << "Priority: Emergency\n";
        } else {
            cout << "Priority: Normal\n";
        }

        cout << "-------------------------\n";
    }

    return 0;
}
```
