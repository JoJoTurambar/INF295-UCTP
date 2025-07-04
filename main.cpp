#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

const string instancesPath = "Inputs/";
const string outputPath = "Outputs/";

const string filenameExtention = ".tim";
const string filenameS = "small_";
const string filenameM = "med_";
const string filenameL = "big_";
const vector<string> sizes = {"S"}; // {"S", "M", "L"}

const short P = 45; // Number of periods
const short D = 5;  // Number of days
const short P_per_D = 9; // Periods per day

// weights
const int w1 = 1;
const int w2 = 1;
const int w3 = 1;

// Hard constraints
// -------------------------------------------------

// //TODO One event per room and period
// bool EventRoomPeriod() {
//     return true;
// }

//TODO Student cannot have 2 events at the same time
bool StudentEventConflict(int numStudents, int numEvents, vector<int> studentEvents, int event, const vector<pair<int, int>>& solution) {
    // event: índice del evento a verificar
    // solution: vector donde solution[e] = {periodo, sala}
    int periodToCheck = solution[event].first;
    if (periodToCheck == -1) return true; // No asignado aún

    // Buscar el estudiante correspondiente a este evento
    // studentEvents está en formato [s0e0, s0e1, ..., s1e0, s1e1, ...]
    // Para cada estudiante s, revisa si tiene más de un evento en el mismo periodo
    for (int s = 0; s < numStudents; ++s) {
        // ¿El estudiante s asiste al evento 'event'?
        if (studentEvents[s * numEvents + event] == 1) {
            // Buscar todos los eventos de este estudiante
            int count = 0;
            for (int e = 0; e < solution.size(); ++e) {
                if (studentEvents[s * numEvents + e] == 1 && solution[e].first == periodToCheck) {
                    count++;
                    if (count > 1) return false; // Más de un evento en el mismo periodo
                }
            }
        }
    }

    return true;
}

// Room must have the characteristics required by the event
bool RoomCharacteristicsCheck(vector<int> roomCharacteristics, vector<int> eventCharacteristics, int event, int room, int numCharacteristics) {
    for (int i = 0; i < numCharacteristics; i++) {
        if (roomCharacteristics[numCharacteristics*room+i] != eventCharacteristics[numCharacteristics*event+i]){
            return false;
        }
    }
    return true;
}

// Amount of students must not exceed the room capacity
bool RoomCapacity(int numStudents, int numEvents, vector<int> studentEvents, vector<int> rooms, int room, int event) {
    int studentsAttendingEvent = 0;
    int stuEventsSize = studentEvents.size();
    for (int i = event; i < stuEventsSize; i = i + numEvents) { // check 1 event for each student
        if (studentEvents[i] == 1) {
            studentsAttendingEvent++;
        }
    }
    // cout << "room " <<   studentsAttendingEvent << endl;
    printf("room %d, capacity %d, students attending %d\n", room, rooms[room], studentsAttendingEvent);
    if (studentsAttendingEvent <= rooms[room]) {
        return true;
    }
    return false;
}

bool HardConstraints(
    vector<pair<int, int>> &solution, int numEvents, int numRooms, int numStudents, 
    vector<int> &studentEvents, vector<int> &rooms, int numCharacteristics, 
    vector<int> &roomCharacteristics, vector<int> &eventCharacteristics, int e, int r
) {
    // Check one event per room and period
    // if (!EventRoomPeriod()) {
    //     return false;
    // }

    // Check student event conflicts
    for (int s = 0; s < numStudents; ++s) {
        if (!StudentEventConflict(numStudents, numEvents, studentEvents, s, solution)) {
            return false;
        }
    }

    // Check room characteristics and capacity for the given event and room
    if (!RoomCharacteristicsCheck(roomCharacteristics, eventCharacteristics, e, r, numCharacteristics)) {
        return false;
    }
    if (!RoomCapacity(numStudents, numEvents, studentEvents, rooms, r, e)) {
        return false;
    }

    return true;
}

// -------------------------------------------------

// Soft constraints. Return 1 if the student meets the condition
// -------------------------------------------------

// Studen must not have an unique event in one day
// int StudentSingleEvent(vector<pair<int, int>> &solution, int numStudents, vector<int> &studentEvents,  int numEvents) {
//     if 
// }

// Student must not have 2 events at the end of the day
// int StudentEventsEndDay() {
//     cout << "not implemented yet" << endl;
//     return 0;
// }

// // Student must not have more than 2 consecutive events
// int Student2ConsecutiveEvents() {
//     cout << "not implemented yet" << endl;
//     return 0;
// }

// Evaluation Function
int EvaluationFunction(
    vector<pair<int, int>> &solution, int numStudents, vector<int> &studentEvents,  int numEvents
) {
    int sge = 0;
    int seed = 0;
    int s2ce = 0;
    int periodEvent, day;
    // SigleEvent
    for (int s = 0; s < numStudents; s++) { // for each student
        vector<int> studentEventsWeek;
        studentEventsWeek.assign(P, 0);

        day = 0; // reset day
        int numEvents1 = 0;
        int numEvents2 = 0;

        // Mark occupied slots in the week of the current stundent
        for (int sEvents = s; sEvents < numEvents; sEvents++) {
            if (studentEvents[sEvents] == 1) {
                periodEvent = solution[sEvents].first; 
                studentEventsWeek[periodEvent] = 1;
                numEvents1++;
            }
        }

        for (auto event : studentEventsWeek) {
            if (event == 1) {
                numEvents2++;
            }
        }

        cout << "studentEvents" << numEvents1 << endl;
        cout << "studentEventsWeek" << numEvents2 << endl;

        // SingleEvent
        int count = 0; // Forma actual contar sólo 1 SingleEvent por estudiante
        for (day = 0; day < 5; day++) { // for each day of the week
            for (int i = day*9; i < (day+1)*9 - 1; i++) { // check periods in the day
                if (studentEventsWeek[i] == 1) {
                    count++;
                }
            }
            if (count == 1) {
                sge++;
                break;
            }

            // EndDay
            if (studentEventsWeek[(day+1)*9-1] == 1) {
                seed++;
            }

            // 2Consecutive
            for (int i = day; i < day*9 - 2; i++) {
                if (studentEventsWeek[i] == 1 && studentEventsWeek[i+1] == 1 && studentEventsWeek[i+2] == 1) {
                    s2ce++;
                }
            }
            cout << "day: " << day << endl;
            count = 0; // reset count for the next day
        }
        if (seed != 0) {
            seed = 1;
        }
        if (s2ce != 0) {
            s2ce = 1;
        }
        cout << "student. " << s << endl;
    }
    // }

    int result = w1 * sge + w2 * seed + w3 * s2ce;
    return result;
}

void Greedy(
    vector<pair<int, int>>& solution, int numEvents, int numRooms, int numStudents, vector<int> &studentEvents, 
    vector<int> &rooms, int numCharacteristics, vector<int> &roomCharacteristics, vector<int> &eventCharacteristics
) {
    // Inicializa la solución vacía
    solution.assign(numEvents, {-1, -1});
    vector<vector<bool>> roomPeriodUsed(numRooms, vector<bool>(P, false));

    for (int e = 0; e < numEvents; ++e) { // For each event
        bool assigned = false; // the event has not been assigned yet
        for (int p = 0; p < P && !assigned; ++p) { // For each period
            for (int r = 0; r < numRooms && !assigned; ++r) { // iterate over rooms and check if the event has not been assigned yet
                // Check hard constraints
                if (
                    HardConstraints(solution, numEvents, numRooms, numStudents, studentEvents, rooms, numCharacteristics, 
                        roomCharacteristics, eventCharacteristics, e, r) &&
                        !roomPeriodUsed[r][p] // Check if the room and period are not already used
                    ) 
                    {
                    solution[e] = {p, r}; // period - room
                    roomPeriodUsed[r][p] = true;
                    assigned = true;
                }
            }
        }
        if (!assigned) {
            // cerr << "Wasn't possible to assign event " << e << endl;
        }
    }
}

//TODO Hill Climbing First Improvement
void HCFI() {
    // Implement the Hill Climbing First Improvement algorithm here
    cout << "Hill Climbing First Improvement not implemented yet." << endl;
    // Implementar movimiento de aleatoriedad
}

string GetFileName(int i, string size) {
    if (size == "S") {
        return filenameS + to_string(i) + filenameExtention;
    }
    else if (size == "M") {
        return filenameM + to_string(i) + filenameExtention;
    }
    else {
        return filenameL + to_string(i) + filenameExtention;
    }
}

void WriteSolution(vector<pair<int, int>> &solution, string &filename) {
    ofstream file(outputPath+filename);
    if (!file.is_open()) {
        cerr << "Error opening output file: " << filename << endl;
        return;
    }

    int length = solution.size();

    for (int i = 0; i < length; i++){ 
        file << solution[i].first << " " << solution[i].second << endl;
    }

    file.close();
}

// Read instance and solve
void SolveInstance(string& fileName) {

    ifstream file(instancesPath+fileName);
    if (!file.is_open()) {
        cerr << "Error opening file: " << fileName << endl;
        return;
    }

    // variables
    int numEvents;
    int numRooms;
    int numCharacteristics;
    int numStudents;

    // Read and assign data to variables
    file >> numEvents >> numRooms >> numCharacteristics >> numStudents;

    vector<int> rooms(numRooms);
    
    for (int i = 0; i < numRooms; i++) {
        file >> rooms[i];
    }

    // variables for par vectors
    vector<int> studentEvents(numStudents*numEvents);
    vector<int> roomCharacteristics(numRooms*numCharacteristics);
    vector<int> eventCharacteristics(numEvents*numCharacteristics);

    // Read par vectors
    for (int i = 0; i < numStudents*numEvents; i++) {
        file >> studentEvents[i];
    }
    for (int i = 0; i < numRooms*numCharacteristics; i++) {
        file >> roomCharacteristics[i];
    }
    for (int i = 0; i < numEvents*numCharacteristics; i++) {
        file >> eventCharacteristics[i];
    }

    file.close();

    // Get initial solution
    vector<pair<int, int>> solution; // periodo sala
    Greedy(solution, numEvents, numRooms, numStudents, studentEvents, rooms, numCharacteristics, roomCharacteristics,
    eventCharacteristics);

    // Apply Hill Climbing First Improvement
    // HCFI();

    WriteSolution(solution, fileName);

    // cout << EvaluationFunction(solution, numStudents, studentEvents, numEvents) << endl;
    // printf("%s: %d\n", fileName.c_str(), EvaluationFunction(solution, numStudents, studentEvents, numEvents));
    
}

int main () {

    string filename;

    //
    // Read all instances
    for (int j = 0; j < 1; j++){
        for (int i = 1; i <= 1; i++) {
            filename = GetFileName(i, sizes[j]);
            // filename = "small_1.tim";
            SolveInstance(filename);
        }
    }

    return 0;
}