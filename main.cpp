

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
void Commit(int);
void InitializeSystem();
void LoadInstructions();


// Constants
const int NUM_REGISTERS = 8; // as in the prompt R0 => R7
const int MEMORY_SIZE = 128 * 1024 / 2;

// Instruction durations
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

struct ReservationStation {
    bool busy = false;
    op OP;
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
};

// Global Variables
vector<Register> registers(NUM_REGISTERS);
vector<ReservationStation> Reserves;  // Initialize according to the number of each type
int memory[MEMORY_SIZE];

queue<Instruction> instructionQueue;


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

            inst.destination_reg = stoi(destRegStr.substr(1));
            inst.rs1 = stoi(rs1.substr(1));
            inst.rs2 = stoi(rs2.substr(1));

            cout << "Instruction: " << inst.OP.type << endl;
            cout << "Rd: " << inst.destination_reg << endl;
            cout << "Rs1: " << inst.rs1 << endl;
            cout << "Rs2: " << inst.rs2 << endl;

            instructionQueue.push(inst);
        }
    }

    // values el registers hena
    for (int i = 0; i < NUM_REGISTERS; ++i) {
        cout << "Register R" << i << ": " << registers[i].value << endl;
    }
}



void Issuing(int currentCycle) {
    if (!instructionQueue.empty()) {
        Instruction& inst = instructionQueue.front();
        
        for (auto& station : Reserves) {
            if (!station.busy) {
                station.busy = true;
                station.OP = inst.OP;
                station.issueTime = currentCycle;

                // b initialize every inst to its default duration
                if (station.OP.type == "LOAD") {
                    station.duration = loadDur;
                    
                } else if (station.OP.type == "STORE") {
                    station.duration = storeDur;
                    
                } else if (station.OP.type == "BNE") {
                    station.duration = bneDuration;
                    
                } else if (station.OP.type == "RET") {
                    station.duration = ReturnDuration;
                    
                } else if (station.OP.type == "ADD" || station.OP.type == "ADDI") {
                    station.duration = AddAddiDuration;
                    
                } else if (station.OP.type == "CALL") {
                    station.duration = CallDuration;
                    
                } else if (station.OP.type == "NAND") {
                    station.duration = nandDuration;
                    
                } else if (station.OP.type == "DIV") {
                    station.duration = DIVDuration;
                    
                } else {
                    station.duration = 1;
                }

                instructionQueue.pop();
                
                break;
            }
            
            else
                continue;
        }
    }
}

void Execute(int currentCycle) {
    for (auto& station : Reserves) {
        if (station.busy && station.issueTime != -1 && station.execCompleteCycle == -1)
        {
            if (station.execStartCycle == -1) {
                // Check if ready
                if ((station.qj == -1 || !registers[station.qj].busy) &&
                    (station.qk == -1 || !registers[station.qk].busy)) {
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


void Commit(int currentCycle) {
    for (auto& station : Reserves) {
        if (station.busy && station.writeCycle != -1) {
            station.busy = false;
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

    Reserves.resize(10);
 
}

void LoadInstructions() {

}


int main() {
    int currentCycle = 0;
    int totalCycles = 100;  // Adjust as necessary

    InitializeRegs();
//    LoadInstructions();

    do {
        Issuing(currentCycle);
        Execute(currentCycle);
        WriteResult(currentCycle);
        Commit(currentCycle);
        currentCycle++;
    }
    while ((currentCycle < totalCycles));
    
    read_and_Print();
    
    return 0;
}

void ADD(int ){
    
    Instruction instr;
    
    registers[instr.destination_reg].value = registers[instr.rs1].value + registers[instr.rs2].value ;
    
}
void ADDI(int ){
    
    Instruction instr;

    registers[instr.destination_reg].value = registers[instr.rs1].value + instr.offset_imm;

}
void LOAD(int ){
    
    
}
void STORE(int ){
    
    
}
void BNE(int ){
    
    
}
void CALL(int ){
    
    
}
void RET(int ){
    
    
}
void DIV(int ){
    
    
}
void NAND(int ){
    
    
}
