#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unordered_map>

using namespace std;
short memory[64000];
int currCycle = 0;
int instIndex = 0;
int branchencountered=0;
int branchtaken=0;
int branchissue=8888888;
bool branch = 0;
bool willFlush=false;
int LOADrs, STORErs, BNErs, CALL_RETrs, ADDERSrs, NANDERSrs, DIVERSrs;
int addC, divC, storeC, loadC, bneC, callretC, nandC;

unordered_map<string, int32_t> labels;

// branch prediction = branchtaken/branchencountered

struct instruction{ //an instruction that saves inside it the registers used and the operation we want to perform
    string op = ""; string fullInst = "";
    int rd = 0; int r1 = 0; int r2 = 0;
    bool offset_r2 = false; bool load_store = false;
    int address = 0; string label;

    //--------------------------------- all the above are instruction properties we will need for the execution of the program
    // what is to come might be variables we need for the execution stage
    int execCycles; //this is a variable based on the instruction that determines how many cycles it will need inside the execution stage
    string stage = "";
    int execS; bool startedExec = false; //variables to let me know if and when i started execution
    int iss = 0; int exe = 0; int wb = 0;
};
struct reg{
    int value = 0;
    bool pending = false;
    int Qi; //to track vj, vk, qj, qk, qi
};
void readFile(string text, vector<instruction>& program, unordered_map<string, int32_t>& labels)
{
    //a function to read from the text file, maybe will decide to have it return an array with every line
    vector<string> to_read;
    ifstream file;
    file.open(text);
    vector<vector<string>> words; //this is used to break down each line into words
    vector<string> lines; //this is used to store the words in these lines

    if (file.is_open()) {                                                              //checking to see if I accessed the file or not to begin with
        cout << "Successfully accessed the Instruction Program" << endl;
    }
    else {
        cout << "!error accessing file!" << endl;
        exit(-1);
    }
    cout << endl;
    string line;                                                                    //temp for each line
    while (!file.eof()) {
        getline(file, line);
        to_read.push_back(line);
    }

    for(auto line : to_read){
        string word = "";
        for (auto x : line) //this loop is to collect each word letter by letter to put seperated words as lines in vector words
        {
            if (x == ' ')
            {
                lines.push_back(word); //push into the line vector each word
                word = ""; //clear the word
            }
            else {
                word = word + x;
            }
        }
        lines.push_back(word);
        words.push_back(lines);
        lines.clear();//clear the array of all words
    }

    for(int i = 0; i < words.size(); i++){
        instruction instance;
        instance.fullInst = to_read[i];
        bool labelfound = false;
        if(words[i][0] == "ADD" || words[i][0] == "ADDI" || words[i][0] == "NAND" || words[i][0] == "DIV"){
            instance.op = words[i][0];
            instance.rd = stoi(words[i][1].substr(1));
            instance.r1 = stoi(words[i][2].substr(1));
            if(words[i][3][0] == 'R') instance.r2 = stoi(words[i][3].substr(1));
            else{
                instance.r2 = stoi(words[i][3]); instance.offset_r2 = 1;
            }
        }else if(words[i][0] == "RET"){
            instance.r1 = 1;
            instance.op = "RET";
        }else if(words[i][0] == "BNE"){
            instance.op = words[i][0];
            instance.r1 = stoi(words[i][1].substr(1));
            instance.r2 = stoi(words[i][2].substr(1));
            instance.label = words[i][3];
        }else if(words[i][0] == "LOAD" || words[i][0] == "STORE")
        { //we have to figure what's before ( to find offset and what's after ( and before ) to get rs1
            instance.op = words[i][0];
            instance.rd = stoi(words[i][1].substr(1));
            instance.load_store = true;
            instance.offset_r2 = true;
            string word;
            for (auto x : words[i][2]) //this loop is to collect each word letter by letter to put seperated words as lines in vector words
            {
                if (x == '(')
                {
                    instance.r2 = stoi(word);
                    word = ""; //clear the word
                }
                else if(x == ')'){
                    instance.r1 = stoi(word.substr(1));
                }
                else{
                    word = word + x;
                }
            }
        }else if(words[i][0] == "CALL"){
            instance.op = words[i][0];
            instance.label = words[i][1];

        }else if(words[i][0].back() == ':'){
            words[i][0].pop_back();
            labels[words[i][0]] = i;
            words[i].erase(words[i].begin()); i--; labelfound = true; //push out the label and read the line again
        }
        else {cout << "UNRECOGNIZED INSTRUCTION" << endl;
            return;
        }

        if(!labelfound) program.push_back(instance);
    }
};

void readMemory(string text){
    vector<string> to_read;
    ifstream file;
    file.open(text);
    vector<vector<string>> words; //this is used to break down each line into words
    vector<string> lines; //this is used to store the words in these lines

    if (file.is_open()) {                                                              //checking to see if I accessed the file or not to begin with
        cout << "Successfully accessed the Memory" << endl;
    }
    else {
        cout << "!error accessing file!" << endl;
        exit(-1);
    }
    cout << endl;
    string line;                                                                    //temp for each line
    while (!file.eof()) {
        getline(file, line);
        to_read.push_back(line);
    }

    for(auto line : to_read){
        string word = "";
        for (auto x : line) //this loop is to collect each word letter by letter to put seperated words as lines in vector words
        {
            if (x == ' ')
            {
                lines.push_back(word); //push into the line vector each word
                word = ""; //clear the word
            }
            else {
                word = word + x;
            }
        }
        lines.push_back(word);
        words.push_back(lines);
        lines.clear();//clear the array of all words
    }

    for(int i = 0; i < 64000; i++){
        memory[i] = 0;
    }

    for(int i = 0; i  < words.size(); i++){
        int index;
        index = stoi(words[i][0]);
        memory[index] = stoi(words[i][1]);
    }


}

void setCycles(vector<instruction>& program){ //this is another function we use to prepare the program for execution
    for(auto& instruct : program){
        if(instruct.op == "ADD" || instruct.op == "ADDI") instruct.execCycles = 2;
        else if(instruct.op == "LOAD" || instruct.op == "STORE") instruct.execCycles = 3;
        else if(instruct.op == "DIV") instruct.execCycles = 10;
        else if(instruct.op == "CALL" || instruct.op == "BNE" || instruct.op == "RET" || instruct.op == "NAND") instruct.execCycles = 1;
    }
}

struct reservationStation //edit to make look different
{
    int Vj, Vk, Qj, Qk, result, address, rd, imm;
    int execS = -10, execE = -10;
    string operation;
    bool executing = false;
    bool writing = false;
    bool busy;
    int unit; //UNIT ID
    int instIndex; //stores index of startexec array etc.
};

vector<reservationStation> ADDERS;
vector<reservationStation> LOADERS, STORES;
vector<reservationStation> DIV, BNE, CALLRET,NAND;
//branchflushing vector
vector<int>MightFlush;

vector<reg> registerStatus(8);
vector<int> registerVal(8, 0);
vector<int> startIss, startExec(100, 0), endExec(100, 0), startWB(100, 0); //arrays to store when instructions stages begin and end


void initializeResStat(){
    for(int i = 0; i < LOADERS.size(); i++){
        LOADERS[i].Qj = -1; LOADERS[i].Qk = -1;
        LOADERS[i].unit = i + 1;
    }
    for(int i = 0; i < STORES.size(); i++){
        STORES[i].Qj = -1; STORES[i].Qk = -1; STORES[i].unit = i + 3;  }
    for(int i = 0; i < BNE.size(); i++){BNE[i].Qj = -1; BNE[i].Qk = -1;
        BNE[i].unit = i + 5 ; }
    for(int i = 0; i < CALLRET.size(); i++){CALLRET[i].Qj = -1; CALLRET[i].Qk = -1;
        CALLRET[i].unit = i + 6; }
    for(int i = 0; i < ADDERS.size(); i++){ADDERS[i].Qj = -1; ADDERS[i].Qk = -1;
        ADDERS[i].unit = i + 7; }
    for(int i = 0; i < NAND.size(); i++){NAND[i].Qj = -1; NAND[i].Qk = -1;
        NAND[i].unit = i + 10;  }
    for(int i = 0; i < DIV.size(); i++){DIV[i].Qj = -1; DIV[i].Qk = -1;
        DIV[i].unit = i + 11;}
}
void updateUnits(int res, int unit){ //updates the units' operands waiting for a RAW
    for(int i = 0; i < ADDERS.size(); i++){
        if(ADDERS[i].Qj == unit){
            ADDERS[i].Qj = 0; ADDERS[i].Vj = res;
        }
        if(ADDERS[i].Qk == unit){
            ADDERS[i].Qk = 0; ADDERS[i].Vk = res;
        }
    }
    for(int i = 0; i < DIV.size(); i++){
        if(DIV[i].Qj == unit){
            DIV[i].Qj = 0; DIV[i].Vj = res;
        }
        if(DIV[i].Qk == unit){
            DIV[i].Qk = 0; DIV[i].Vk = res;
        }
    }

    for(int i = 0; i < NAND.size(); i++){
        if(NAND[i].Qj == unit){
            NAND[i].Qj = 0; NAND[i].Vj = res;
        }
        if(NAND[i].Qk == unit){
            NAND[i].Qk = 0; NAND[i].Vk = res;
        }
    }

    for(int i = 0; i < LOADERS.size(); i++){
        if(LOADERS[i].Qj == unit){
            LOADERS[i].Qj = 0; LOADERS[i].Vj = res;
        }
        if(LOADERS[i].Qk == unit){
            LOADERS[i].Qk = 0; LOADERS[i].Vk = res;
        }
    }
    for(int i = 0; i < STORES.size(); i++){
        if(STORES[i].Qj == unit){
            STORES[i].Qj = 0; STORES[i].Vj = res;
        }
        if(STORES[i].Qk == unit){
            STORES[i].Qk = 0; STORES[i].Vk = res;
        }
        if (STORES[i].execE == currCycle && STORES[i].Qk == 0)

        {
            memory[STORES[i].address] = STORES[i].Vk;

        }
    }
}
void updateRegs(int result, int unit){ //function used to update operands waiting for a RAW
    for(int i = 0; i < registerStatus.size(); i++){
        if(registerStatus[i].Qi == unit){
            registerVal[i] = result;
            registerStatus[i].Qi = 0;
        }
    }
}
void issue(vector<instruction>& program) {
    if (instIndex < program.size()) {
        if (program[instIndex].op == "LOAD") {
            int emptyRes = -1;

            for (int i = 0; i < LOADERS.size(); i++) {
                if (!(LOADERS[i].busy)) {
                    emptyRes = i;
                    break;
                }
            }


            if (emptyRes >= 0) { //if we did find an empty reservation station to issue to, save its index
                if (registerStatus[program[instIndex].r1].Qi >
                    0) {  //if the register r1 is waiting for input from another station/instrcution, RAW
                    LOADERS[emptyRes].Qj = registerStatus[program[instIndex].r1].Qi;
                } else if (registerStatus[program[instIndex].r1].Qi == 0) { //otherwise the register is free
                    LOADERS[emptyRes].Vj = registerVal[program[instIndex].r1];
                    LOADERS[emptyRes].Qj = 0;
                }

                if (branch) {
                    MightFlush.push_back(LOADERS[emptyRes].instIndex);
                }

                registerStatus[program[instIndex].rd].Qi = LOADERS[emptyRes].unit; //to know the unit that the rd is waiting for output from
                LOADERS[emptyRes].address = program[instIndex].r2; //as we store immediate in r2 as well
                LOADERS[emptyRes].busy = true;
                LOADERS[emptyRes].operation = "LOAD";

                startIss.push_back(currCycle);
                LOADERS[emptyRes].instIndex = startIss.size() - 1;

                LOADERS[emptyRes].rd = program[instIndex].rd;
                instIndex++;


            }
        } else if (program[instIndex].op == "STORE") {
            int emptyRes = -1;
            for (int i = 0; i < STORES.size(); i++) {
                if (!(STORES[i].busy)) {
                    emptyRes = i;
                    break;
                }
            }
            if (emptyRes >= 0) { //if we did find an empty reservation station to issue to, save its index
                if (registerStatus[program[instIndex].r1].Qi >
                    0) {  //if the register r1 is waiting for input from another station/instrcution, RAW
                    STORES[emptyRes].Qj = registerStatus[program[instIndex].r1].Qi;
                } else if (registerStatus[program[instIndex].r1].Qi == 0) { //otherwise the register is free
                    STORES[emptyRes].Vj = registerVal[program[instIndex].r1];
                    STORES[emptyRes].Qj = 0;
                }

                if (branch) {
                    MightFlush.push_back(STORES[emptyRes].instIndex);
                }

                registerStatus[program[instIndex].rd].Qi = STORES[emptyRes].unit; //to know the unit that the rd is waiting for output from
                STORES[emptyRes].address = program[instIndex].r2; //as we store immediate in r2 as well
                STORES[emptyRes].busy = true;
                STORES[emptyRes].operation = "STORE";

                startIss.push_back(currCycle);
                STORES[emptyRes].instIndex = startIss.size() - 1;

                STORES[emptyRes].rd = program[instIndex].rd;
                instIndex++;

            }
        } else if (program[instIndex].op == "ADD" || program[instIndex].op == "ADDI") {
            int emptyRes = -1;
            for (int i = 0; i < ADDERS.size(); i++) {
                if (!(ADDERS[i].busy)) {
                    emptyRes = i;
                    break;
                }
            }
            if (emptyRes >= 0) { //if we did find an empty reservation station to issue to, save its index

                if (registerStatus[program[instIndex].r1].Qi >
                    0) {  //if the register r1 is waiting for input from another station/instrcution, RAW
                    ADDERS[emptyRes].Qj = registerStatus[program[instIndex].r1].Qi;
                } else if (registerStatus[program[instIndex].r1].Qi == 0) { //otherwise the register is free
                    ADDERS[emptyRes].Vj = registerVal[program[instIndex].r1];
                    ADDERS[emptyRes].Qj = 0;
                }

                if (registerStatus[program[instIndex].r2].Qi > 0) {  //checking for RAW for register 2
                    ADDERS[emptyRes].Qk = registerStatus[program[instIndex].r2].Qi;
                } else if (registerStatus[program[instIndex].r2].Qi == 0) { //otherwise the register is free
                    ADDERS[emptyRes].Vk = registerVal[program[instIndex].r2];
                    ADDERS[emptyRes].Qk = 0;
                }

                if (program[instIndex].op == "ADD")
                    ADDERS[emptyRes].operation = "ADD";

                else
                    ADDERS[emptyRes].operation = "ADDI";

                if (branch) {
                    MightFlush.push_back(ADDERS[emptyRes].instIndex);
                }
                ADDERS[emptyRes].busy = true;
                registerStatus[program[instIndex].rd].Qi = ADDERS[emptyRes].unit; //to know the unit that the rd is waiting for output from
                startIss.push_back(currCycle);

                ADDERS[emptyRes].instIndex = startIss.size() - 1;
                ADDERS[emptyRes].rd = program[instIndex].rd;
                instIndex++;
            }

        } else if (program[instIndex].op == "NAND") {

            int emptyRes = -1;
            for (int i = 0; i < NAND.size(); i++) {
                if (!(NAND[i].busy)) {
                    emptyRes = i;
                    break;
                }
            }
            if (emptyRes >= 0) { // If an empty reservation station is found
                if (registerStatus[program[instIndex].r1].Qi > 0) { // Check for RAW on r1
                    NAND[emptyRes].Qj = registerStatus[program[instIndex].r1].Qi;
                } else {
                    NAND[emptyRes].Vj = registerVal[program[instIndex].r1];
                    NAND[emptyRes].Qj = 0;
                }

                if (registerStatus[program[instIndex].r2].Qi > 0) { // Check for RAW on r2
                    NAND[emptyRes].Qk = registerStatus[program[instIndex].r2].Qi;
                } else {
                    NAND[emptyRes].Vk = registerVal[program[instIndex].r2];
                    NAND[emptyRes].Qk = 0;
                }

                if (branch) {
                    MightFlush.push_back(NAND[emptyRes].instIndex);
                }
                // Set up reservation station for NAND
                NAND[emptyRes].operation = "NAND";
                NAND[emptyRes].busy = true;
                registerStatus[program[instIndex].rd].Qi = NAND[emptyRes].unit; // Mark rd as waiting for this unit

                startIss.push_back(currCycle);

                NAND[emptyRes].instIndex = startIss.size() - 1;
                NAND[emptyRes].rd = program[instIndex].rd;
                instIndex++;
                //            cout<<"NAND rd"<<NAND[emptyRes].rd;
            }


        } else if (program[instIndex].op == "DIV") {
            int emptyRes = -1;
            for (int i = 0; i < DIV.size(); i++) {
                if (!(DIV[i].busy)) {
                    emptyRes = i;
                    break;
                }
            }
            if (emptyRes >= 0) { // If an empty reservation station is found
                if (registerStatus[program[instIndex].r1].Qi > 0) { // Check for RAW on r1
                    DIV[emptyRes].Qj = registerStatus[program[instIndex].r1].Qi;

                } else {
                    DIV[emptyRes].Vj = registerVal[program[instIndex].r1];
                    DIV[emptyRes].Qj = 0;

                }

                if (registerStatus[program[instIndex].r2].Qi > 0) { // Check for RAW on r2
                    DIV[emptyRes].Qk = registerStatus[program[instIndex].r2].Qi;
                } else {
                    DIV[emptyRes].Vk = registerVal[program[instIndex].r2];
                    DIV[emptyRes].Qk = 0;
                }

                if (branch) {
                    MightFlush.push_back(DIV[emptyRes].instIndex);
                }
                DIV[emptyRes].operation = "DIV";
                DIV[emptyRes].busy = true;
                registerStatus[program[instIndex].rd].Qi = DIV[emptyRes].unit; // Mark rd as waiting for this unit

                startIss.push_back(currCycle);

                DIV[emptyRes].instIndex = startIss.size() - 1;
                DIV[emptyRes].rd = program[instIndex].rd;
                instIndex++;
            }

        } else if (program[instIndex].op == "BNE") {
            int emptyRes = -1;
            for (int i = 0; i < BNE.size(); i++) {
                if (!(BNE[i].busy)) {
                    emptyRes = i;
                    break;
                }
            }
            if (emptyRes >= 0) { // If an empty reservation station is found
                if (registerStatus[program[instIndex].r1].Qi > 0) { // Check for RAW on r1
                    BNE[emptyRes].Qj = registerStatus[program[instIndex].r1].Qi;

                } else {
                    BNE[emptyRes].Vj = registerVal[program[instIndex].r1];
                    BNE[emptyRes].Qj = 0;

                }

                if (registerStatus[program[instIndex].r2].Qi > 0) { // Check for RAW on r2
                    BNE[emptyRes].Qk = registerStatus[program[instIndex].r2].Qi;
                } else {
                    BNE[emptyRes].Vk = registerVal[program[instIndex].r2];
                    BNE[emptyRes].Qk = 0;
                }

                BNE[emptyRes].operation = "BNE";
                BNE[emptyRes].busy = true;
                registerStatus[program[instIndex].rd].Qi = BNE[emptyRes].unit; // Mark rd as waiting for this unit

                startIss.push_back(currCycle);

                BNE[emptyRes].instIndex = startIss.size() - 1;

                BNE[emptyRes].rd = labels[program[instIndex].label];
                branch = true;
                branchissue = instIndex;
                branchencountered++;
                instIndex++;
            }
        } else if (program[instIndex].op == "CALL" || program[instIndex].op == "RET") {
            int emptyRes = -1;
            for (int i = 0; i < CALLRET.size(); i++) {
                if (!(CALLRET[i].busy)) {
                    emptyRes = i;
                    break;
                }
            }
            if (emptyRes >= 0) { // If an empty reservation station is found
                if (program[instIndex].op == "CALL") { // case CALL

                    CALLRET[emptyRes].operation = "CALL";
                    CALLRET[emptyRes].busy = true;
                    startIss.push_back(currCycle);

                    CALLRET[emptyRes].instIndex = startIss.size() - 1;

                    CALLRET[emptyRes].address = labels[program[instIndex].label];
                    branch = true;
                    instIndex++;

                } else if (program[instIndex].op == "RET") { //case RET

                    CALLRET[emptyRes].operation = "RET";
                    CALLRET[emptyRes].busy = true;
                    startIss.push_back(currCycle);

                    CALLRET[emptyRes].instIndex = startIss.size() - 1;

                    registerStatus[1].Qi = DIV[emptyRes].unit;

                    CALLRET[emptyRes].address = labels[program[instIndex].label];
                    branch = true;
                    instIndex++;
                }


            }
        }
    }
}
void execute(){


    for(int i = 0; i < ADDERS.size(); i++){
        if (ADDERS[i].operation=="ADD") {
            if(ADDERS[i].Qj == 0 && ADDERS[i].Qk == 0 && !(ADDERS[i].executing) && ((!branch) || (branch && startIss[ADDERS[i].instIndex] < branchissue ))){

                ADDERS[i].execS = currCycle;
                ADDERS[i].execE = currCycle + addC;
                ADDERS[i].executing = 1;
                startExec[ADDERS[i].instIndex] = currCycle;
                endExec[ADDERS[i].instIndex] = currCycle + addC;
                ADDERS[i].result = ADDERS[i].Vk + ADDERS[i].Vj;
            }
        } else if (ADDERS[i].operation=="ADDI"){

            if(ADDERS[i].Qj == 0 && ADDERS[i].Qk == 0 && !(ADDERS[i].executing)&& ((!branch) || (branch && startIss[ADDERS[i].instIndex] < branchissue ))){

                ADDERS[i].execS = currCycle;
                ADDERS[i].execE = currCycle + addC;
                ADDERS[i].executing = 1;
                startExec[ADDERS[i].instIndex] = currCycle;
                endExec[ADDERS[i].instIndex] = currCycle + addC;
                ADDERS[i].result = ADDERS[i].Vk + ADDERS[i].imm;

            }
        }

    }

    for (int i = 0; i < NAND.size(); i++) {
        if (NAND[i].operation == "NAND") {
            if (NAND[i].Qj == 0 && NAND[i].Qk == 0 && !NAND[i].executing&& ((!branch) || (branch && startIss[NAND[i].instIndex] < branchissue ))) {
                NAND[i].execS = currCycle;
                NAND[i].execE = currCycle + nandC;
                NAND[i].executing = 1;
                startExec[NAND[i].instIndex] = currCycle;
                endExec[NAND[i].instIndex] = currCycle + nandC;
                NAND[i].result = ~(NAND[i].Vk & NAND[i].Vj);
            }
        }

    }

    for (int i = 0; i < DIV.size(); i++) {
        if (DIV[i].operation == "DIV") {
            //            cout<<"Qk"<<DIV[i].Qk<<endl;
            if (DIV[i].Qj == 0 && DIV[i].Qk == 0 && !DIV[i].executing&& ((!branch) || (branch && startIss[DIV[i].instIndex] < branchissue ))) {
                DIV[i].execS = currCycle;
                DIV[i].execE = currCycle + divC;
                DIV[i].executing = 1;
                startExec[DIV[i].instIndex] = currCycle;
                endExec[DIV[i].instIndex] = currCycle + divC;
                //              DIV[i].result = DIV[i].Vk / DIV[i].Vj;
            }
        }

    }

    for (int i = 0; i < LOADERS.size(); i++) {
        if (LOADERS[i].operation == "LOAD") {
            if (LOADERS[i].Qj == 0  && !LOADERS[i].executing&& ((!branch) || (branch && startIss[LOADERS[i].instIndex] < branchissue ))) {
                LOADERS[i].execS = currCycle;
                LOADERS[i].execE = currCycle + loadC;
                LOADERS[i].executing = 1;
                startExec[LOADERS[i].instIndex] = currCycle;
                endExec[LOADERS[i].instIndex] = currCycle + loadC;
            }

            LOADERS[i].address = LOADERS[i].address + LOADERS[i].Vj;
            LOADERS[i].result = memory[LOADERS[i].address];
        }

    }
    for (int i = 0; i < STORES.size(); i++) {
        if (STORES[i].operation == "STORE") {
            if (STORES[i].Qj == 0 && !STORES[i].executing && ((!branch) || (branch && startIss[STORES[i].instIndex] < branchissue )))
            {
                STORES[i].execS = currCycle;
                STORES[i].execE = currCycle + storeC;
                STORES[i].executing = 1;
                startExec[STORES[i].instIndex] = currCycle;
                endExec[STORES[i].instIndex] = currCycle + storeC;
            }
            STORES[i].address = STORES[i].address + STORES[i].Vj;
            STORES[i].result = registerVal[STORES[i].rd];
        }

    }
    for (int i = 0; i < BNE.size(); i++) {
        if (BNE[i].operation == "BNE") {
            if (BNE[i].Qj == 0 && BNE[i].Qk == 0 && !BNE[i].executing) {
                BNE[i].execS = currCycle;
                BNE[i].execE = currCycle + bneC;
                BNE[i].executing = 1;
                startExec[BNE[i].instIndex] = currCycle;
                endExec[BNE[i].instIndex] = currCycle + bneC;

                if (BNE[i].Vj!=BNE[i].Vk)
                    BNE[i].result=1;
                else
                    BNE[i].result=0;

                if (BNE[i].result==1) {

                    willFlush=1;
                } else willFlush=0;
            }

            if((endExec[BNE[i].instIndex]==currCycle) || (abs((instIndex - BNE[i].rd))==currCycle)){
                branch=0;
                willFlush=0;
                branchissue=8888888;
                if (BNE[i].result==1) {
                    instIndex=startIss[BNE[i].instIndex]+1+BNE[i].rd;
                    branchtaken++;
                }
            }
        }
    }
    for (int i = 0; i < CALLRET.size(); i++) {
        if (CALLRET[i].operation == "CALL") {
            if (CALLRET[i].Qj == 0 && CALLRET[i].Qk == 0 && !CALLRET[i].executing&& ((!branch) || (branch && startIss[LOADERS[i].instIndex] < branchissue ))) {
                CALLRET[i].execS = currCycle;
                CALLRET[i].execE = currCycle + callretC;
                CALLRET[i].executing = 1;
                startExec[CALLRET[i].instIndex] = currCycle;
                endExec[CALLRET[i].instIndex] = currCycle + callretC;
                willFlush=1;

            }


            if(endExec[CALLRET[i].instIndex]==currCycle){

                branch=0;
                willFlush=0;
                branchissue=8888888;
                instIndex=instIndex+1+CALLRET[i].address;

            }
        }
        else if (CALLRET[i].operation == "RET") {
            if (CALLRET[i].Qj == 0 && CALLRET[i].Qk == 0 && !CALLRET[i].executing&& ((!branch) || (branch && startIss[LOADERS[i].instIndex] < branchissue ))) {
                CALLRET[i].execS = currCycle;
                CALLRET[i].execE = currCycle + callretC;
                CALLRET[i].executing = 1;
                startExec[CALLRET[i].instIndex] = currCycle;
                endExec[CALLRET[i].instIndex] = currCycle + callretC;
                willFlush=1;
            }

            if(endExec[CALLRET[i].instIndex]==currCycle){

                branch=0;
                willFlush=0;
                branchissue=8888888;
                instIndex=instIndex+1+CALLRET[i].rd;

            }
        }

    }
}


void writeBack(){

    for(int i = 0; i < ADDERS.size(); i++){
        if(endExec[ADDERS[i].instIndex] + 1 <= currCycle && ADDERS[i].executing && ADDERS[i].busy){
            //            index = i;
            //            out = ADDERS[i].unitType;

            //releasing the reservation station, updating the registers and dependencies
            updateRegs(ADDERS[i].result, ADDERS[i].unit);
            updateUnits(ADDERS[i].result, ADDERS[i].unit);
            ADDERS[i].busy = false;
            startWB[ADDERS[i].instIndex] = currCycle;
            ADDERS[i].executing = false;
            ADDERS[i].Qj = -1;
            ADDERS[i].Qk = -1;
            ADDERS[i].Vj = -1;
            ADDERS[i].Vk = -1;
            if(ADDERS[i].rd != 0){
                registerVal[ADDERS[i].rd] = ADDERS[i].result;
            }
        }
    }
    for (int i=0; i<NAND.size();i++){
        if (endExec[NAND[i].instIndex] +1 <= currCycle && NAND[i].executing && NAND[i].busy) {
            updateRegs(NAND[i].result, NAND[i].unit);
            updateUnits(NAND[i].result, NAND[i].unit);
            NAND[i].busy = false;
            startWB[NAND[i].instIndex] = currCycle;
            NAND[i].executing = false;
            NAND[i].Qj = -1;
            NAND[i].Qk = -1;
            NAND[i].Vj = -1;
            NAND[i].Vk = -1;
            if (ADDERS[i].rd!=0){
                registerVal[NAND[i].rd] = NAND[i].result;

            }
        }
    }

    for (int i=0; i<DIV.size();i++){
        if (endExec[DIV[i].instIndex] +1 <= currCycle && DIV[i].executing && DIV[i].busy) {
            updateRegs(DIV[i].result, DIV[i].unit);
            updateUnits(DIV[i].result, DIV[i].unit);
            DIV[i].busy = false;
            startWB[DIV[i].instIndex] = currCycle;
            DIV[i].executing = false;
            DIV[i].Qj = -1;
            DIV[i].Qk = -1;
            DIV[i].Vj = -1;
            DIV[i].Vk = -1;
            if (DIV[i].rd!=0){

                registerVal[DIV[i].rd] = DIV[i].result;
            }
        }
    }
    for (int i = 0 ; i<LOADERS.size(); i++) {
        if(endExec[LOADERS[i].instIndex]+1 <= currCycle && LOADERS[i].executing &&LOADERS[i].busy){

            updateRegs(LOADERS[i].result, LOADERS[i].unit);
            updateUnits(LOADERS[i].result, LOADERS[i].unit);
            LOADERS[i].busy=false;
            startWB[LOADERS[i].instIndex]=currCycle;
            LOADERS[i].executing = false;
            LOADERS[i].Qj = -1;
            LOADERS[i].Qk = -1;
            LOADERS[i].Vj = -1;
            LOADERS[i].Vk = -1;


            if (LOADERS[i].rd!=0)
            {

                registerVal[LOADERS[i].rd] = LOADERS[i].result;
            }
        }
    }

    for (int i = 0 ; i<STORES.size(); i++) {
        if(endExec[STORES[i].instIndex]+1 <= currCycle && STORES[i].executing &&STORES[i].busy){

            updateRegs(STORES[i].result, STORES[i].unit);
            updateUnits(STORES[i].result, STORES[i].unit);
            STORES[i].busy=false;
            startWB[STORES[i].instIndex] = currCycle;
            STORES[i].executing = false;
            STORES[i].Qj = -1;
            STORES[i].Qk = -1;
            STORES[i].Vj = -1;
            STORES[i].Vk = -1;


            memory[STORES[i].address] = STORES[i].result;
        }
    }
    for (int i = 0 ; i<BNE.size(); i++) {
        if(endExec[BNE[i].instIndex]+1 <= currCycle && BNE[i].executing && BNE[i].busy){

            updateRegs(BNE[i].result, BNE[i].unit);
            updateUnits(BNE[i].result, BNE[i].unit);
            BNE[i].busy=false;
            startWB[BNE[i].instIndex] = currCycle;
            BNE[i].executing = false;
            BNE[i].Qj = -1;
            BNE[i].Qk = -1;
            BNE[i].Vj = -1;
            BNE[i].Vk = -1;

            if (willFlush==1) {

                for (int i=0; i<MightFlush.size(); i++) {
                    startExec[MightFlush[i]]=0; endExec[MightFlush[i]]=0; startWB[MightFlush[i]]=0; // if branch is taken we will flush all next instructions
                }
                MightFlush.clear(); // to clear the flushed instructions
            }
        }
    }
    for (int i = 0 ; i<CALLRET.size(); i++) {
        if(endExec[CALLRET[i].instIndex]+1 <= currCycle && CALLRET[i].executing && CALLRET[i].busy){

            updateRegs(CALLRET[i].result, CALLRET[i].unit);
            updateUnits(CALLRET[i].result, CALLRET[i].unit);
            CALLRET[i].busy=false;
            startWB[CALLRET[i].instIndex] = currCycle;
            CALLRET[i].executing = false;
            CALLRET[i].Qj = -1;
            CALLRET[i].Qk = -1;
            CALLRET[i].Vj = -1;
            CALLRET[i].Vk = -1;

            if (willFlush==1) {

                for (int i=0; i<MightFlush.size(); i++) {
                    startExec[MightFlush[i]]=0; endExec[MightFlush[i]]=0; startWB[MightFlush[i]]=0; // if branch is taken we will flush all next instructions
                }
                MightFlush.clear(); // to clear the flushed instructions
            }
        }
    }
}

// Function to print Tomasulo's table
void printTomasulosTable(const vector<instruction>& program) {

    cout << left << setw(15) << "Instruction"
         << setw(12) << "Issue"
         << setw(15) << "Start Exec"
         << setw(15) << "End Exec"
         << setw(12) << "Write Back" << endl;

    for (int i = 0; i < program.size(); i++) {
        cout << left << setw(15) << program[i].fullInst
             << setw(12) << startIss[i]
             << setw(15) << startExec[i]
             << setw(15) << endExec[i]
             << setw(12) << startWB[i] << endl;
    }
}




int main() {
    cout << "Input numbers of reservation stations for adding: " ; cin >> ADDERSrs;
    cout << "Input numbers of reservation stations for BNE: " ; cin >> BNErs;
    cout << "Input numbers of reservation stations for division: " ; cin >> DIVERSrs;
    cout << "Input numbers of reservation stations for NAND: " ; cin >> NANDERSrs;
    cout << "Input numbers of reservation stations for CALL/RET: " ; cin >> CALL_RETrs;
    cout << "Input numbers of reservation stations for loading: " ; cin >> LOADrs;
    cout << "Input numbers of reservation stations for storing: " ; cin >> STORErs;
//int addC, divC, storeC, loadC, bneC, callretC, nandC;
    cout << "Input cycles for addition: "; cin >> addC;
    cout << "Input cycles for divison: "; cin >> divC;
    cout << "Input cycles for BNE: "; cin >> bneC;
    cout << "Input cycles for loading: "; cin >> loadC;
    cout << "Input cycles for storing: "; cin >> storeC;
    cout << "Input cycles for NAND: "; cin >> nandC;
    cout << "Input cycles for CALL/RET: "; cin >> callretC;
    registerVal[2] = 10; registerVal[3] = 20; registerVal[1] = 5;

    ADDERS.resize(ADDERSrs); STORES.resize(STORErs); LOADERS.resize(LOADrs); BNE.resize(BNErs);
    DIV.resize(DIVERSrs); NAND.resize(NANDERSrs); CALLRET.resize(CALL_RETrs);
    double max = -100;

    vector<instruction> program;
    readMemory("/Users/muhammadabdelmohsen/Desktop/CE Projects/Computer Arch Project/Arch-project2/Memory.txt");
    readFile( "/Users/muhammadabdelmohsen/Desktop/CE Projects/Computer Arch Project/Arch-project2/TESTcases3.txt",program, labels);
    setCycles(program);

    initializeResStat();
    while(currCycle < 100){
        issue(program);
        currCycle++;
        execute();
        writeBack();
        execute();
    }
    printTomasulosTable(program); // Add this line to call the print function

    for(int i = 0; i < startWB.size(); i++) if(max < startWB[i]) max = startWB[i];
    double IPC = program.size()/max;

    cout << "IPC = " << IPC << endl;
    cout<<"Total execution Time: "<<max<<endl;
    if(branchencountered!=0){
        cout<<"Percentage of Branch Misprediction = "<<(branchtaken/branchencountered)*100<<endl;
    } else
        cout<<"No Branch is encountered"<<endl;

    return 0;

}
//unordered_map<string, instruction> find_insts; //map that maybe i will need for execution later, an idea bs for now



