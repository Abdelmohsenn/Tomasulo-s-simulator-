

#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;


void Issuing(int);
void Execute(int);
void WriteResult(int);
void InitializeSystem();
void LoadInstructions();


// Constants
const int NUM_REGISTERS = 8; // as in the prompt R0 => R7
const int MEMORY_SIZE = 128 * 1024 / 2; // 128 kb mem

// Static Instruction durations assumptions
const int loadDur = 3;   // 1 (compute address) + 2 (read from memory)
const int storeDur = 3;  // 1 (compute address) + 2 (write to memory)
const int bneDuration = 1;
const int ReturnDuration = 1;
const int AddAddiDuration = 2;
const int CallDuration = 1;
const int nandDuration = 1;
const int DIVDuration = 10;

// for tracking
int totalInstructions = 0;
int completedInstructions = 0;
int currentCycle = 0;

// inst name
struct op {
    string type;
};

class ReservationStation {
public:
    bool busy = false;
    op OP;
    int stations;  // Number of stations for this type
    int vj = -1;
    int vk = -1;
    int qj = -1;
    int qk = -1;
    int Address = -1;
    int issueTime = -1;
    int execStartCycle = -1;
    int execCompleteCycle = -1;
    int writeCycle = -1;
    int duration = -1;

};

class ReservationStationsCount {
public:
    vector<ReservationStation> ADDRES;
    vector<ReservationStation> ADDIRES;
    vector<ReservationStation> STORERES;
    vector<ReservationStation> LOADRES;
    vector<ReservationStation> RETRES;
    vector<ReservationStation> CALLRES;
    vector<ReservationStation> BNERES;
    vector<ReservationStation> NANDRES;
    vector<ReservationStation> DIVRES;

    ReservationStationsCount() {
        // Initialize stations bl sizes
        ADDRES.resize(3);
        ADDIRES.resize(3);
        STORERES.resize(2);
        LOADRES.resize(2);
        RETRES.resize(1);
        CALLRES.resize(1);
        BNERES.resize(1);
        NANDRES.resize(1);
        DIVRES.resize(1);
    }
};


struct Instruction {
    op OP;
    int destination_reg;
    int rs1;
    int rs2;
    int offset_imm;
};

struct Register {
    int value;
    bool busy = false;
    string state = "Free"; // idk how to use this still
};

// Global Variables
vector<Register> registers(NUM_REGISTERS);

vector<ReservationStation> Reserves;  // da el station el adeem. kan total. idk law lesa hnhtago
int memory[MEMORY_SIZE]; //main mem

queue<Instruction> instructionQueue; // queue of instructions


    
    void read_and_Print() {
        ifstream file;
        string line;
        string destRegStr;
        string rs1;
        string rs2;

        string path = "/Users/muhammadabdelmohsen/Desktop/CE Projects/Computer Arch Project/Arch-project2/Tumasulo's.txt";
        file.open(path);

        if (!file.is_open()) {
            cerr << "Error opening file: " << path << endl;
            return;
        } else {
            
            while (getline(file, line)) {
                istringstream reader(line);
                Instruction inst;

                reader >> inst.OP.type;
                reader >> destRegStr;
                reader >> rs1;
                reader >> rs2;

                inst.destination_reg = stoi(destRegStr.substr(1)); // tany index mn awl reg
                inst.rs1 = stoi(rs1.substr(1)); // tany index mn tany reg
                inst.rs2 = stoi(rs2.substr(1)); // tany index mn talet reg

                instructionQueue.push(inst); // putting the instructions in the queue to use later on
            }
        }

        // values el registers hena
        for (int i = 0; i < NUM_REGISTERS; ++i) {
            cout << "Register R" << i << ": " << registers[i].value << endl;
        }

        // test el queue gowah eh
        
//        cout << "Loaded Instructions:" << endl;
//        queue<Instruction> tempQueue = instructionQueue; // Copy the queue for printing
//        while (!tempQueue.empty()) {
//            Instruction inst = tempQueue.front();
//            cout << "Instruction: " << inst.OP.type << inst.destination_reg
//                  << inst.rs1  << inst.rs2 << endl;
//            tempQueue.pop();
//        }
    }




void Issuing(int currentCycle) {
    if (!instructionQueue.empty()) {
        Instruction& inst = instructionQueue.front();
        ReservationStationsCount stationCount;
        
        // Check the instruction type and use the corresponding reservation stations
        if (inst.OP.type == "ADD" || inst.OP.type == "ADDI") {
            for (auto& station : stationCount.ADDRES) {
                if (!station.busy) {
                    // Check other conditions for ADD and ADDI operations
                    station.busy = true;
                    station.OP = inst.OP;
                    station.issueTime = currentCycle;
                    station.duration =  AddAddiDuration;
                    instructionQueue.pop();
                    break;
                }
            }
        }
        else if (inst.OP.type  == "ADDI") {
            for (auto& station : stationCount.ADDRES) {
                if (!station.busy) {
                    // Check other conditions for ADD and ADDI operations
                    station.busy = true;
                    station.OP = inst.OP;
                    station.issueTime = currentCycle;
                    station.duration =  AddAddiDuration;
                    instructionQueue.pop();
                    break;
                }
            }
        }
        else if (inst.OP.type == "LOAD") {
            for (auto& station : stationCount.LOADRES) {
                if (!station.busy) {
                    // Check other conditions for LOAD operations
                    station.busy = true;
                    station.OP = inst.OP;
                    station.issueTime = currentCycle;
                    station.duration = loadDur;
                    instructionQueue.pop();
                    break;
                }
            }
            
        }
    }
}

void Execute(int currentCycle) {
    for (auto& station : Reserves) {
        if (station.busy && station.issueTime != -1 && station.execCompleteCycle == -1)
        {
            if (station.execStartCycle == -1) {
                // Check if ready
                if ((station.qj == -1 || !registers[station.qj].busy) && (station.qk == -1 || !registers[station.qk].busy))
                {
                    station.execStartCycle = currentCycle;
                }
            }

            if (currentCycle - station.execStartCycle == station.duration) {
                station.execCompleteCycle = currentCycle;
            }
        }
    }
}


void WriteResult(int currentCycle) {
    for (auto& station : Reserves) {
        if (station.busy && station.execCompleteCycle != -1 && station.writeCycle == -1) {
            station.writeCycle = currentCycle;
        }
    }
}


void InitializeRegs() {
    for (int i = 0; i < NUM_REGISTERS; ++i) {
        registers[i].value = 0;
        registers[i].busy = false;
    }
    registers[0].value=0;
    registers[0].busy=true; // 3ashan mesh 3ayzo Yetghayar
 
}

void LoadInstructions() {

}


int main() {
    int currentCycle = 0;
    int totalCycles = 100;

    InitializeRegs();// initialize regs b zeros
    ReservationStationsCount stationCount;
    
    
// test count el stations
    cout << "ADD stations: " << stationCount.ADDRES.size() << endl;
    cout << "LOAD stations: " << stationCount.LOADRES.size() << endl;
    cout << "STORE stations: " << stationCount.STORERES.size() << endl;
    cout << "ADDI stations: " << stationCount.ADDIRES.size() << endl;
    cout << "RET stations: " << stationCount.RETRES.size() << endl;
    cout << "CALL stations: " << stationCount.CALLRES.size() << endl;
    cout << "DIV stations: " << stationCount.DIVRES.size() << endl;
    cout << "BNE stations: " << stationCount.ADDRES.size() << endl;
    cout << "NAND stations: " << stationCount.ADDRES.size() << endl;


    do {
        Issuing(currentCycle);
        Execute(currentCycle);
        WriteResult(currentCycle);
        currentCycle++;
    }
    while ((currentCycle < totalCycles));
    
    read_and_Print();
    
    return 0;
}


