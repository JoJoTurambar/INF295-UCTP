// main.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <numeric>   // For std::iota
#include <algorithm> // For std::shuffle
#include <random>    // For std::mt19937

using namespace std;

// --- Global Constants and Configuration ---
const string instancesPath = "Inputs/";
const string outputPath = "Outputs/";

const string filenameExtention = ".tim";
const string filenameS = "small_";
const string filenameM = "med_";
const string filenameL = "big_";
const vector<string> sizes = {"S"}; // {"S", "M", "L"}

const short P = 45; // Number of periods (5 days * 9 periods/day)
const short D = 5;  // Number of days
const short P_per_D = 9; // Periods per day

// Soft constraint weights from your model
const float w1 = 1.0f; // Single event in a day
const float w2 = 1.0f; // More than 2 consecutive events
const float w3 = 1.0f; // Event in the last slot of a day

// --- Data Structures ---
// A solution assigns a pair {period, room} to each event
using Solution = vector<pair<int, int>>;

// --- Forward Declarations ---
int calculateObjective(const Solution& solution, int numStudents, int numEvents, const vector<int>& studentEvents);

// --- Hard Constraint Checks ---

// This single function checks if assigning an 'event' to a 'period' and 'room' is valid,
// considering the current state of the 'solution'.
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
    // 1. One event per room and period
    // This is the fastest check, so we do it first.
    if (roomPeriodUsed[room][period]) {
        return false;
    }

    // 2. Room must have the characteristics required by the event
    // CORRECTED LOGIC: if event requires a feature, the room must have it.
    for (int c = 0; c < numCharacteristics; ++c) {
        bool event_requires_feature = (eventCharacteristics[event * numCharacteristics + c] == 1);
        bool room_has_feature = (roomCharacteristics[room * numCharacteristics + c] == 1);
        if (event_requires_feature && !room_has_feature) {
            return false;
        }
    }

    // 3. Amount of students must not exceed the room capacity
    int studentsAttendingEvent = 0;
    for (int s = 0; s < numStudents; ++s) {
        if (studentEvents[s * numEvents + event] == 1) {
            studentsAttendingEvent++;
        }
    }
    if (studentsAttendingEvent > rooms[room]) {
        return false;
    }

    // 4. A student cannot have two events at the same time
    // CORRECTED LOGIC: Check only students attending the *current* event.
    for (int s = 0; s < numStudents; ++s) {
        // If student 's' attends the event we are trying to place...
        if (studentEvents[s * numEvents + event] == 1) {
            // ...then check all *other* events this student is enrolled in.
            for (int other_e = 0; other_e < numEvents; ++other_e) {
                if (event == other_e) continue; // Don't compare event to itself

                // If student 's' is also in 'other_e' and 'other_e' is scheduled at the same period...
                if (studentEvents[s * numEvents + other_e] == 1 && solution[other_e].first == period) {
                    return false; // Conflict found
                }
            }
        }
    }

    // If all checks pass, the assignment is feasible
    return true;
}


// --- Greedy Construction Heuristic ---
void Greedy(
    Solution& solution,
    int numEvents, int numRooms, int numStudents, int numCharacteristics,
    const vector<int>& rooms,
    const vector<int>& studentEvents,
    const vector<int>& roomCharacteristics,
    const vector<int>& eventCharacteristics)
{
    solution.assign(numEvents, {-1, -1});
    vector<vector<bool>> roomPeriodUsed(numRooms, vector<bool>(P, false));

    // For better solutions, randomize the order in which events are processed.
    // This helps Hill Climbing later by starting from different points.
    vector<int> eventOrder(numEvents);
    iota(eventOrder.begin(), eventOrder.end(), 0); // Fills with 0, 1, 2,...
    random_device rd;
    mt19937 g(rd());
    shuffle(eventOrder.begin(), eventOrder.end(), g);

    for (int e : eventOrder) {
        bool assigned = false;
        for (int p = 0; p < P && !assigned; ++p) {
            for (int r = 0; r < numRooms && !assigned; ++r) {
                // Use the new, clean feasibility check
                if (isFeasible(e, p, r, solution, numEvents, numRooms, numStudents, numCharacteristics,
                               rooms, roomPeriodUsed, studentEvents, roomCharacteristics, eventCharacteristics))
                {
                    solution[e] = {p, r};
                    roomPeriodUsed[r][p] = true;
                    assigned = true;
                }
            }
        }
        if (!assigned) {
            cerr << "Warning: Could not assign event " << e << ". The solution will be infeasible." << endl;
        }
    }
}


// --- Hill Climbing First Improvement ---
void HillClimbingFirstImprovement(
    Solution& solution,
    int numEvents, int numRooms, int numStudents, int numCharacteristics,
    const vector<int>& rooms,
    const vector<int>& studentEvents,
    const vector<int>& roomCharacteristics,
    const vector<int>& eventCharacteristics)
{
    cout << "Starting Hill Climbing..." << endl;
    int currentCost = calculateObjective(solution, numStudents, numEvents, studentEvents);
    cout << "Initial Cost: " << currentCost << endl;

    bool improvementFound;
    do {
        improvementFound = false;

        // Iterate through all events to find a move
        for (int e = 0; e < numEvents; ++e) {
            // Store the original assignment of the event
            pair<int, int> originalAssignment = solution[e];
            if (originalAssignment.first == -1) continue; // Skip unassigned events

            // Try to move this event to every other possible slot
            for (int p = 0; p < P; ++p) {
                for (int r = 0; r < numRooms; ++r) {
                    if (p == originalAssignment.first && r == originalAssignment.second) {
                        continue; // Skip the current position
                    }

                    // Temporarily "un-assign" the event to check feasibility
                    solution[e] = {-1, -1};
                    
                    // Build a room-period usage map for the *current* state of the solution
                    vector<vector<bool>> roomPeriodUsed(numRooms, vector<bool>(P, false));
                    for(int i=0; i<numEvents; ++i) {
                        if(solution[i].first != -1) {
                            roomPeriodUsed[solution[i].second][solution[i].first] = true;
                        }
                    }

                    if (isFeasible(e, p, r, solution, numEvents, numRooms, numStudents, numCharacteristics,
                                   rooms, roomPeriodUsed, studentEvents, roomCharacteristics, eventCharacteristics))
                    {
                        // If feasible, apply the move and evaluate
                        solution[e] = {p, r};
                        int newCost = calculateObjective(solution, numStudents, numEvents, studentEvents);

                        if (newCost < currentCost) {
                            // First Improvement found!
                            currentCost = newCost;
                            improvementFound = true;
                            cout << "Found improvement. New cost: " << currentCost << endl;
                            goto next_iteration; // Break all loops and restart the search
                        }
                    }
                }
            }
            // If no better move was found for this event, revert it to its original state
            solution[e] = originalAssignment;
        }

    next_iteration:;
    } while (improvementFound);

    cout << "Hill Climbing finished. Final cost: " << currentCost << endl;
}


// --- Evaluation (Objective Function) ---
// REWRITTEN FOR CLARITY AND CORRECTNESS
int calculateObjective(const Solution& solution, int numStudents, int numEvents, const vector<int>& studentEvents) {
    float totalPenalty = 0.0f;

    for (int s = 0; s < numStudents; ++s) {
        // For each student, find out which periods they have an event in
        vector<int> studentSchedule;
        for (int e = 0; e < numEvents; ++e) {
            if (solution[e].first != -1 && studentEvents[s * numEvents + e] == 1) {
                studentSchedule.push_back(solution[e].first);
            }
        }
        sort(studentSchedule.begin(), studentSchedule.end());

        if (studentSchedule.empty()) {
            continue;
        }

        // --- Calculate Penalties for this student ---

        // 1. Penalty for having a single event on a day
        int singleEventDays = 0;
        for (int d = 0; d < D; ++d) {
            int eventsOnDay = 0;
            int dayStartPeriod = d * P_per_D;
            int dayEndPeriod = dayStartPeriod + P_per_D;
            for (int period : studentSchedule) {
                if (period >= dayStartPeriod && period < dayEndPeriod) {
                    eventsOnDay++;
                }
            }
            if (eventsOnDay == 1) {
                singleEventDays++;
            }
        }

        // 2. Penalty for having more than 2 consecutive events
        int threeConsecutive = 0;
        for (size_t i = 0; i + 2 < studentSchedule.size(); ++i) {
            if (studentSchedule[i+1] == studentSchedule[i] + 1 && studentSchedule[i+2] == studentSchedule[i] + 2) {
                threeConsecutive++;
            }
        }

        // 3. Penalty for an event in the last slot of a day
        int lastSlotEvents = 0;
        for (int period : studentSchedule) {
            // Last slots are 8, 17, 26, 35, 44
            if ((period + 1) % P_per_D == 0) {
                lastSlotEvents++;
            }
        }

        totalPenalty += (w1 * singleEventDays) + (w2 * threeConsecutive) + (w3 * lastSlotEvents);
    }

    return static_cast<int>(totalPenalty);
}

// --- File I/O and Main Logic ---

string GetFileName(int i, const string& size) {
    if (size == "S") return filenameS + to_string(i) + filenameExtention;
    if (size == "M") return filenameM + to_string(i) + filenameExtention;
    return filenameL + to_string(i) + filenameExtention;
}

void WriteSolution(const Solution& solution, const string& filename) {
    ofstream file(outputPath + filename);
    if (!file.is_open()) {
        cerr << "Error opening output file: " << outputPath + filename << endl;
        return;
    }

    for (const auto& assignment : solution) {
        file << assignment.first << " " << assignment.second << endl;
    }
    file.close();
}

void SolveInstance(const string& fileName) {
    cout << "\n--- Solving instance: " << fileName << " ---" << endl;
    ifstream file(instancesPath + fileName);
    if (!file.is_open()) {
        cerr << "Error opening file: " << instancesPath + fileName << endl;
        return;
    }

    int numEvents, numRooms, numCharacteristics, numStudents;
    file >> numEvents >> numRooms >> numCharacteristics >> numStudents;

    vector<int> rooms(numRooms);
    for (int i = 0; i < numRooms; ++i) file >> rooms[i];

    vector<int> studentEvents(numStudents * numEvents);
    for (int i = 0; i < numStudents * numEvents; ++i) file >> studentEvents[i];

    vector<int> roomCharacteristics(numRooms * numCharacteristics);
    for (int i = 0; i < numRooms * numCharacteristics; ++i) file >> roomCharacteristics[i];

    vector<int> eventCharacteristics(numEvents * numCharacteristics);
    for (int i = 0; i < numEvents * numCharacteristics; ++i) file >> eventCharacteristics[i];

    file.close();

    // 1. Get initial solution using Greedy
    Solution solution;
    Greedy(solution, numEvents, numRooms, numStudents, numCharacteristics,
           rooms, studentEvents, roomCharacteristics, eventCharacteristics);

    cout << "Greedy solution generated." << endl;
    int initialCost = calculateObjective(solution, numStudents, numEvents, studentEvents);
    printf("Initial solution objective value: %d\n", initialCost);
    
    // 2. Improve the solution using Hill Climbing
    HillClimbingFirstImprovement(solution, numEvents, numRooms, numStudents, numCharacteristics,
                                 rooms, studentEvents, roomCharacteristics, eventCharacteristics);

    // 3. Write final solution
    WriteSolution(solution, fileName);
    cout << "Solution written to " << outputPath + fileName << endl;
}

int main() {
    for (const auto& size : sizes) {
        for (int i = 1; i <= 1; i++) { // Adjust loop for number of instances
            string filename = GetFileName(i, size);
            SolveInstance(filename);
        }
    }
    return 0;
}