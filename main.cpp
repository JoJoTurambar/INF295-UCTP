#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>

using namespace std;

// Global constants for reading instances and writing outputs
const string instancesPath = "Inputs/";
const string outputPath = "Outputs/";

const string filenameExtention = ".tim";
const string filenameS = "small_";
const string filenameM = "med_";
const string filenameL = "big_";
const vector<string> sizes = {"S", "M", "L"};

// Periods and days
const short P = 45; // Number of periods
const short D = 5;  // Number of days
const short P_per_D = 9; // Periods per day

const int MAX_ITERATIONS = 100;
// const int MAX_ITERATIONS_M = 100;
// const int MAX_ITERATIONS_L = 10; 

// weights
const int w1 = 1;
const int w2 = 1;
const int w3 = 4;

using Solution = vector<pair<int, int>>; // period - room

int EvaluationFunction(const Solution& solution, int numStudents, int numEvents, const vector<int>& studentEvents);

// Check hard constraints
bool isFeasible(
    int event, int period, int room,
    const Solution& solution,
    int numEvents, int numRooms, int numStudents, int numCharacteristics,
    const vector<int>& rooms,
    const vector<vector<bool>>& roomPeriodUsed,
    const vector<int>& studentEvents,
    const vector<int>& roomCharacteristics,
    const vector<int>& eventCharacteristics)
{
    // One event per room and period
    if (roomPeriodUsed[room][period]) {
        return false;
    }

    // Room must have the characteristics required by the event
    for (int c = 0; c < numCharacteristics; ++c) {
        bool event_requires_feature = (eventCharacteristics[event * numCharacteristics + c] == 1);
        bool room_has_feature = (roomCharacteristics[room * numCharacteristics + c] == 1);
        if (event_requires_feature && !room_has_feature) {
            return false;
        }
    }

    // Amount of students must not exceed the room capacity
    int studentsAttendingEvent = 0;
    for (int s = 0; s < numStudents; ++s) {
        if (studentEvents[s * numEvents + event] == 1) {
            studentsAttendingEvent++;
        }
    }
    if (studentsAttendingEvent > rooms[room]) {
        return false;
    }

    // A student cannot have two events at the same time
    for (int s = 0; s < numStudents; ++s) {
        if (studentEvents[s * numEvents + event] == 1) { // If s is attendint the event
            for (int other_e = 0; other_e < numEvents; ++other_e) { // Check other events
                if (event == other_e) continue; // Don't compare event to itself

                if (studentEvents[s * numEvents + other_e] == 1 && solution[other_e].first == period) {
                    return false;
                }
            }
        }
    }

    return true;
}

// Evaluation Function
int EvaluationFunction(
    const Solution& solution, int numStudents, int numEvents, const vector<int>& studentEvents
) {
    int result = 0;

    for (int s = 0; s < numStudents; s++) { // for each student
        vector<int> studentSchedule;

        for (int e = 0; e < numEvents; ++e) { // for each event
            // Check if the event is assigned and if the student attends it
            if (solution[e].first != -1 && studentEvents[s * numEvents + e] == 1) {
                studentSchedule.push_back(solution[e].first);
            }
        }
        sort(studentSchedule.begin(), studentSchedule.end()); // sort periods

        int studentScheduleSize = studentSchedule.size();

        // if the student has no events, continue to the next student
        if (studentSchedule.empty()) {
            continue;
        }

        // Penalties
        int singleEventDays = 0;
        int threeConsecutive = 0;
        int lastPeriodEvents = 0;
        for (int d = 0; d < D; ++d) { // D = 5 days
            vector<int> periodsOnDay;
            int eventsOnDay = 0;
            int dayStartPeriod = d * P_per_D; // if d = 2: dayStartPeriod = 2 * 9 = 18
            int dayEndPeriod = dayStartPeriod + P_per_D; // P_per_D = 9

            for (int period : studentSchedule) {
                if (period >= dayStartPeriod && period < dayEndPeriod) {
                    eventsOnDay++;
                    periodsOnDay.push_back(period - dayStartPeriod); // 0,1,...,8
                }
            }

            sort(periodsOnDay.begin(), periodsOnDay.end());

            // Add Singe Event
            if (eventsOnDay == 1) {
                singleEventDays++;
            }

            // Add 3 consecutive event
            
            for (int i = 0; i + 2 < studentScheduleSize; ++i) {
                if (studentSchedule[i+1] == studentSchedule[i] + 1 && studentSchedule[i+2] == studentSchedule[i] + 2) {
                    threeConsecutive++;
                }
            }

            // Add EndDay
            if (!periodsOnDay.empty() && (periodsOnDay.back() + 1) % P_per_D == 0) { // last period of the day
                lastPeriodEvents++;
            }
        }

        result += w1 * singleEventDays + w2 * lastPeriodEvents + w3 * threeConsecutive;
    }

    return result;
}



void Greedy(
    Solution& solution, int numEvents, int numRooms, int numStudents, vector<int> &studentEvents, 
    vector<int> &rooms, int numCharacteristics, vector<int> &roomCharacteristics, vector<int> &eventCharacteristics,
    bool randomize
) {
    // Inicializa la solución vacía
    solution.assign(numEvents, {-1, -1});
    vector<vector<bool>> roomPeriodUsed(numRooms, vector<bool>(P, false));

    vector<int> eventOrder(numEvents);
    iota(eventOrder.begin(), eventOrder.end(), 0); // Fills with 0, 1, 2,...
    // Randomize for HCFI
    if (randomize) {
        random_device rd;
        mt19937 g(rd());
        shuffle(eventOrder.begin(), eventOrder.end(), g);
    }

    for (int e : eventOrder) { // For each event
        bool assigned = false; // the event has not been assigned yet
        for (int p = 0; p < P && !assigned; ++p) { // For each period
            for (int r = 0; r < numRooms && !assigned; ++r) { // iterate over rooms and check if the event has not been assigned yet
                // Check hard constraints
                if (isFeasible(e, p, r, solution, numEvents, numRooms, numStudents, numCharacteristics,
                               rooms, roomPeriodUsed, studentEvents, roomCharacteristics, eventCharacteristics))
                    {
                    solution[e] = {p, r}; // period - room
                    roomPeriodUsed[r][p] = true;
                    assigned = true;
                }
            }
        }
        // if (!assigned) {
        //     cerr << "Wasn't possible to assign event " << e << endl;
        // }
    }
}

void HCFI(
    Solution& solution,
    int numEvents, int numRooms, int numStudents, int numCharacteristics,
    vector<int>& rooms,
    vector<int>& studentEvents,
    vector<int>& roomCharacteristics,
    vector<int>& eventCharacteristics)
{
    cout << endl << "Starting Hill Climbing...\n" << endl;
    int currentCost = EvaluationFunction(solution, numStudents, numEvents, studentEvents);
    cout << "Initial Cost: " << currentCost << endl;
    int iterations = 0;

    bool improvementFound;
    do {
        improvementFound = false;
        Solution newSolution;
        Greedy(newSolution, numEvents, numRooms, numStudents, studentEvents, 
            rooms, numCharacteristics, roomCharacteristics, eventCharacteristics, true);
        int newCost = EvaluationFunction(newSolution, numStudents, numEvents, studentEvents);
        if (newCost < currentCost) {
            // If the new solution is better, update the current solution
            solution = newSolution;
            currentCost = newCost;
            improvementFound = true;
            iterations++;
            cout << "Improvement found! New cost: " << currentCost << " after " << iterations << " iterations." << endl;
        } else {
            cout << "No improvement found. Current cost: " << currentCost << endl;
            
        }

    } while (improvementFound && iterations < MAX_ITERATIONS);

    cout << "Hill Climbing finished. Final cost: " << currentCost << endl;
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

void WriteSolution(const Solution &solution, string &filename) {
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
    Solution solution; // periodo sala
    Greedy(solution, numEvents, numRooms, numStudents, studentEvents, rooms, numCharacteristics, roomCharacteristics,
    eventCharacteristics, false);

    // int initialCost = EvaluationFunction(solution, numStudents, numEvents, studentEvents);
    printf("\nFile : %s\n", fileName.c_str());

    // Apply Hill Climbing First Improvement
    HCFI(solution, numEvents, numRooms, numStudents, numCharacteristics,rooms, studentEvents, roomCharacteristics, eventCharacteristics);

    WriteSolution(solution, fileName);
}

int main () {

    string filename;

    //
    // Read all instances
    for (int j = 0; j < 3; j++){
        for (int i = 1; i <= 20; i++) {
            filename = GetFileName(i, sizes[j]);
            // filename = "small_1.tim";
            SolveInstance(filename);
        }
    }

    return 0;
}