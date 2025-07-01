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
const vector<string> sizes = {"S", "M", "L"};

const short P = 45; // Number of periods

// weights
const float w1 = 1;
const float w2 = 1;
const float w3 = 1;

// Hard constraints
// -------------------------------------------------

// One event per room and period
bool EventRoomPeriod() {
    return true;
}

// Student cannot have 2 events at the same time
bool StudentEventConflict(int numStudents, vector<int> studentEvents, int event) {

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
bool RoomCapacity(int numStudents, vector<int> studentEvents, vector<int> rooms, int room, int event) {
    int studentsAttendingEvent = 0;
    for (int i = event; i < studentEvents.size(); i = i + numStudents) {
        if (studentEvents[i] == 1) {
            studentsAttendingEvent++;
        }
    }
    // cin >> studentsAttendingEvent;
    if (studentsAttendingEvent <= rooms[room]) {
        return true;
    }
    return false;
}

// -------------------------------------------------

// Soft constraints. Return 1 if the student meets the condition
// -------------------------------------------------

// Studen must not have an unique event in one day
// int StudentSingleEvent(vector<pair<int, int>> &solution, int numStudents, vector<int> &studentEvents,  int numEvents) {
//     if 
// }

// Student must not have 2 events at the end of the day
int StudentEventsEndDay() {
    cout << "not implemented yet" << endl;
    return 0;
}

// Student must not have more than 2 consecutive events
int Student2ConsecutiveEvents() {
    cout << "not implemented yet" << endl;
    return 0;
}

// Evaluation Function
int  EvaluationFunction(
    vector<pair<int, int>> &solution, int numStudents, vector<int> &studentEvents,  int numEvents
) {
    int sge = 0;
    int seed = 0;
    int s2ce = 0;
    int periodEvent;
    // SigleEvent
    for (int s = 0; s < numStudents; s++) { // for each student
        vector<int> studentEventsInADay = {-1, -1, -1, -1, -1, -1, -1, -1, -1};
        for (int day = 0; day < 5; day++){ // each day
            for (int i = day*9; i < (day+1) * 9; i++) { // each period
                for (int sEvents = s; sEvents < numEvents; sEvents++) {
                    if (studentEvents[sEvents] == 1) {
                        periodEvent = solution[sEvents].first; // aquí period va desde 0 a 44, no hasta 9 como quiero
                        studentEventsInADay[periodEvent] = 1; //hacer esto para toda la semana 
                    }
                }
            }

            // SingleEvent
            int count = 0;
            for (int i = day; i < day*9; i++) {
                if (studentEventsInADay[i] == 1) {
                    count++;
                }
            }
            if (count == 1) {
                sge++;
            }

            // EndDay
            if (studentEventsInADay[day*9-1] == 1) {
                seed++;
            }

            // 2Consecutive
            for (int i = day; i < day*9 - 2; i++) {
                if (studentEventsInADay[i] == 1 && studentEventsInADay[i+1] == 1&& studentEventsInADay[i+2] == 1) {
                    s2ce++;
                }
            }

        }
    }

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

    for (int e = 0; e < numEvents; ++e) {
        bool assigned = false;
        for (int p = 0; p < P && !assigned; ++p) {
            for (int r = 0; r < numRooms && !assigned; ++r) {
                // Check hard constraints
                if (!roomPeriodUsed[r][p] && 
                    RoomCapacity(numStudents, studentEvents, rooms, r, e) &&
                    RoomCharacteristicsCheck(roomCharacteristics, eventCharacteristics, e, r, numCharacteristics) &&
                    StudentEventConflict(numStudents, studentEvents, e) &&
                    EventRoomPeriod()) 
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
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    for (int i = 0; i < solution.size(); i++){ 
        file << solution[i].first << " " << solution[i].second << endl;
    }

    file.close();
}

//TODO Read instance and solve
void SolveInstance(string& fileName) {

    cout << fileName << endl;

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
    
}

int main () {

    string filename;

    // Read all instances
    for (int j = 0; j < 3; j++){
        for (int i = 1; i <= 20; i++) {
            filename = GetFileName(i, sizes[j]);
            SolveInstance(filename);
        }
    }

    return 0;
}