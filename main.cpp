

#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;


//void Issuing(int);
//void Execute(int);
//void WriteResult(int);
void InitializeSystem();


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
bool addwritten=false;

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
    int result=0;
    int destination_reg = -1;

};

class ReservationStationsCount {
public:
    vector<ReservationStation> ADDRES;
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
queue<Instruction> instructionQueue; // queue of instructions


struct Register {
    int value;
    bool busy = false;
    int Qi=-1;
};

// Global Variables
vector<Register> registers(NUM_REGISTERS);

vector<ReservationStation> Reserves;  // da el station el adeem. kan total. idk law lesa hnhtago
int memory[MEMORY_SIZE]; //main mem



    
void read_and_Print() {
    ifstream file;
    string line;
    string destination_reg;
    string rs1;
    string rs2;
    string path = "/Users/muhammadabdelmohsen/Desktop/CE Projects/Computer Arch Project/Arch-project2/Tumasulo's.txt";  // Update with the correct file path
    file.open(path);

    if (!file.is_open()) {
        cerr << "Error opening file: " << path << endl;
        return;
    } else {
        while (getline(file, line)) {
            istringstream reader(line);
            Instruction inst;

            reader >> inst.OP.type;
            reader >> destination_reg;
            reader >> rs1;
            reader>>rs2;
            
            // Check if rs2 starts with "R" (indicating it's a register)
            if (rs2[0] == 'R') {
                inst.rs2 = stoi(rs2.substr(1));
            } else {
                // If it doesn't start with "R," treat it as an immediate value
                inst.offset_imm = stoi(rs2);
            }

            inst.rs1 = stoi(rs1.substr(1));
            inst.destination_reg = stoi(destination_reg.substr(1));


            instructionQueue.push(inst);
//            cout << "Pushed Instruction: " << inst.OP.type << ", Queue Size Now: " << instructionQueue.size() << endl;
        }
    }
    // Printing from the queue itself
    
//    cout << "Instructions in Queue:" << endl;
//    while (!instructionQueue.empty()) {
//        Instruction inst = instructionQueue.front();
//        cout << "Instruction: " << inst.OP.type << " R" << inst.destination_reg << ", R" << inst.rs1 << ", R" << inst.rs2 << endl;
//        cout<<"Size:" << instructionQueue.size()<<endl;
//        instructionQueue.pop();
//
//    }
}





void Issuing(int& currentCycle, ReservationStationsCount& stationCount, int *size) {
    if (!instructionQueue.empty()) {
        Instruction inst = instructionQueue.front();
    
        if (inst.OP.type == "ADD" || inst.OP.type == "ADDI") {
            auto& stations = stationCount.ADDRES;
            *size = stations.size();
            for (int i = 0; i < *size; i++) {
                if (!stations[i].busy) {
                    bool rs1Ready = !registers[inst.rs1].busy || registers[inst.rs1].Qi == -1;
                    bool rs2Ready = true;
                    
                    if ((inst.OP.type == "ADD" && registers[inst.rs2].Qi != -1) || (inst.OP.type == "ADD" && registers[inst.rs2].busy)){
                        rs2Ready = false;
                    }
                    
                    if (inst.OP.type == "ADDI") {
                        rs2Ready = true;  // Immediate value is always ready
                    }
                    
                    if (rs1Ready && rs2Ready) {
                        stations[i].busy = true;
                        stations[i].OP = inst.OP;
                        if (registers[inst.rs1].Qi != -1) {
                            stations[i].qj = registers[inst.rs1].Qi;
                        } else {
                            stations[i].vj = registers[inst.rs1].value;
                            stations[i].qj = 0;
                        }
                        
                        if (inst.OP.type == "ADD") {
                            if (registers[inst.rs2].Qi != -1) {
                                stations[i].qk = registers[inst.rs2].Qi;
                            } else {
                                stations[i].vk = registers[inst.rs2].value;
                                stations[i].qk = 0;
                            }
                        } else if (inst.OP.type == "ADDI") {
                            stations[i].vk = inst.offset_imm;
                            stations[i].qk = 0;  // Immediate value is always ready
                        }
                        
                        stations[i].issueTime = currentCycle;
                        stations[i].duration = AddAddiDuration;
                        stations[i].destination_reg = inst.destination_reg;
                        
                        registers[inst.destination_reg].busy = true;
                        registers[inst.destination_reg].Qi = i;
                        
                        //                        cout<<"Cycles taken in the issuing"<<stations[i].duration;
//                        cout<<*size<<endl;
                        *size = *size +1;
//                        cout<<*size<<endl;
                        instructionQueue.pop();
                        break;
                    }
                    
                }
            }
        }
        
        if (inst.OP.type == "LOAD") {
            auto& stations = stationCount.LOADRES;
            for (int i = 0; i < stations.size(); ++i) {
                if (!stations[i].busy) {
                    bool rs1Ready = !registers[inst.rs1].busy || registers[inst.rs1].Qi == -1;
                    //                    bool rs2Ready = true;
                    
                    
                    
                    if (rs1Ready) {
                        stations[i].busy = true;
                        stations[i].OP = inst.OP;
                        if (registers[inst.rs1].Qi != -1) {
                            stations[i].qj = registers[inst.rs1].Qi;
                        } else {
                            stations[i].vj = registers[inst.rs1].value;
                            stations[i].qj = 0;
                        }
                        
                        stations[i].vk = inst.offset_imm;
                        stations[i].qk = 0;  // Immediate value is always ready
                        
                        stations[i].issueTime = currentCycle;
                        stations[i].duration = loadDur;
                        stations[i].destination_reg = inst.destination_reg;
                        
                        registers[inst.destination_reg].busy = true;
                        registers[inst.destination_reg].Qi = i;
                        
//                        cout<< "Immediate value =>> "<< stations[i].vk <<endl;
//                        cout<< "Rd  =>> "<< stations[i].vj <<endl;
//                        cout <<"this instruction durtion = >>" <<loadDur<<endl;
                        instructionQueue.pop();
                        break;
                    }
                    
                }
            }
        }
    }
}






void Execute(int& currentCycle, ReservationStationsCount& stationCount, int *size) {

    for (auto& station : stationCount.ADDRES) {
        // Ensure that the station is busy, an ADD instruction has been issued, and it has not yet executed
        if (station.busy && station.OP.type == "ADD" && station.issueTime != -1 && station.execCompleteCycle == -1) {

            if (station.qj == 0 && station.qk == 0) {
                station.result = station.vj + station.vk;

                station.execStartCycle= station.issueTime+1;
                station.execCompleteCycle =station.execStartCycle + AddAddiDuration;
                *size = *size - 1;
                            station.busy=false;

                
//                cout<<"current cycle = >"<<currentCycle;
            }
//            cout<<" issue time hena "<<station.issueTime<<endl;

//            cout<<"Result"<<station.result<<endl;;
        }
        else if (station.busy && station.OP.type == "ADDI" && station.issueTime != -1 && station.execCompleteCycle == -1) {
            if (station.qj == 0 && station.qk == 0) {
                station.result = station.vj + station.vk;
                
                // computing el cycle
                station.execStartCycle= station.issueTime+1;
                station.execCompleteCycle =station.execStartCycle + AddAddiDuration;
                *size = *size - 1;


                
//                station.cur
//                cout<< "current cycle = >" << currentCycle;
            }
//            cout<<"Result"<<station.result<<endl;;
//            cout<<"exec cycles hena"<<station.execCompleteCycle<<endl;
//            cout<<"curr cycles hena"<<currentCycle<<endl;
            
            
        }
    }
}

void WriteResult(int& currentCycle, ReservationStationsCount& stationCount,int* size) {
    
    for (auto& station : stationCount.ADDRES) {
        if (station.OP.type == "ADD" && station.busy && station.execCompleteCycle != -1 && station.writeCycle == -1) {

            if (registers[station.destination_reg].Qi == (&station - &stationCount.ADDRES[0])) {
                registers[station.destination_reg].value = station.result;
                registers[station.destination_reg].Qi = -1;
                station.writeCycle=station.execCompleteCycle+1;
//                currentCycle=station.writeCycle;
//                station.busy=false;
                addwritten=true;
//                *size=*size-1;
                
//    cout<<"Current Cycle in the write result = >"<<currentCycle<<endl;
            }
            
        }
        else if (station.OP.type == "ADDI" && station.busy && station.execCompleteCycle != -1 && station.writeCycle == -1) {

            if (registers[station.destination_reg].Qi == (&station - &stationCount.ADDRES[0])) {
                registers[station.destination_reg].value = station.result;
                registers[station.destination_reg].Qi = -1;
                station.writeCycle=station.execCompleteCycle+1;

//                currentCycle=station.writeCycle;
                
    //  cout<<"Current Cycle in the write result = >"<<currentCycle<<endl;

            }
        }
        
    }
    
    
//    cout<<"curr cycles hena"<<currentCycle<<endl;
}





void InitializeRegs() {
    for (int i = 0; i < NUM_REGISTERS; ++i) {
        registers[i].value = 0;
        registers[i].busy = false;
        registers[i].Qi = -1;

    }
    registers[0].value=0;
    registers[0].busy=true; // 3ashan mesh 3ayzo Yetghayar
    
    registers[2].value = 10; // Example value for R2
    registers[3].value = 20; // Example value for R3
 
}

void LoadInstructions() {

}


void PrintResults() {
    cout << "Final Register Values:" << endl;
    for (int i = 0; i < NUM_REGISTERS; ++i) {
        cout << "Register R" << i << ": " << registers[i].value << endl;
    }
}



int main() {
    InitializeRegs();
    read_and_Print();
    int size=0;

    ReservationStationsCount stationCount;
    int currentCycle = 0;
    int totalCycles = 50;

    while (currentCycle < totalCycles) {
        if (!instructionQueue.empty()) {
            cout<<instructionQueue.front().OP.type<< " "<<instructionQueue.front().destination_reg <<endl;
            Issuing(currentCycle, stationCount, &size);
//            cout<<instructionQueue.front().OP.type<< " "<<instructionQueue.front().destination_reg <<endl;
//            instructionQueue.pop();

        }
        Execute(currentCycle, stationCount, &size);
        WriteResult(currentCycle, stationCount, &size);
        size = 0;
        currentCycle++; // Increment the cycle at the end of each loop iteration
    }
    // test count el stations
    //    cout << "ADD stations: " << stationCount.ADDRES.size() << endl;
    //    cout << "LOAD stations: " << stationCount.LOADRES.size() << endl;
    //    cout << "STORE stations: " << stationCount.STORERES.size() << endl;
    //    cout << "RET stations: " << stationCount.RETRES.size() << endl;
    //    cout << "CALL stations: " << stationCount.CALLRES.size() << endl;
    //    cout << "DIV stations: " << stationCount.DIVRES.size() << endl;
    //    cout << "BNE stations: " << stationCount.ADDRES.size() << endl;
    //    cout << "NAND stations: " << stationCount.ADDRES.size() << endl;

    //    cout << "Current Cycle: " << currentCycle << ", Queue Size: " << instructionQueue.size() << endl;

    cout << "Current Cycle at the end: " << currentCycle << endl;
    PrintResults();
    return 0;
}
