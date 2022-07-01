/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <cstdio>
#include <iostream>
using namespace std;

int cyc_counter = 0, num_of_instructions = 0;
int cyc_number_blocked = 0,cyc_number_fg = 0 ;
int threads_num = SIM_GetThreadsNum();
int *load_arr, *store_arr;
bool *load_flags, *store_flags, *halted_flags;
uint32_t *line_per_thread;
tcontext *register_file_per_thread;
int32_t * dst_value;
Instruction * instruction;

/* Initialize the all global arrays */
void zero_arr(){
    for (int i = 0 ; i < threads_num ; i++){
        line_per_thread[i] = 0;
        load_arr[i] = 0;
        load_flags[i] = false;
        store_arr[i] = 0;
        store_flags[i] = false;
        halted_flags[i] = false;
        for (int j = 0 ; j < REGS_COUNT ; j++){
            register_file_per_thread[i].reg[j] = 0;
        }
    }
}

/* Find and return the next thread in the queue that is not halted*/
int get_next_not_halted_thread(int thread){
    int tid = thread;
    if(tid == threads_num-1)
        tid = 0;
    else
        tid++;
    for(int i = 0 ; i < threads_num ;++i) {
        if (!(halted_flags[tid])) {
            return tid;
        }
        if(tid == threads_num-1) tid = 0;
        else tid++;
    }
    return -1;
}

/* Update load and store flags arrays
 * Return false when idle
 * Used for CORE_BlockedMT function
 */
bool check_another_free_thread(int load_latency, int store_latency ){
    for(int tid = 0 ; tid < threads_num ;++tid) {
        if (load_flags[tid] && ((cyc_counter - load_arr[tid] >= load_latency)))
                load_flags[tid] = false; // Update flags if no long busy
    }

    for(int tid = 0 ; tid < threads_num ;++tid) {
        if (store_flags[tid] && (cyc_counter - store_arr[tid] >= store_latency))
                store_flags[tid] = false; // Update flags if no long busy
    }

    for(int tid = 0 ; tid < threads_num ;++tid) {
        if (!(load_flags[tid] || store_flags[tid] || halted_flags[tid]))
                return true;
    }
    return false;
}

/* Find and return the next thread in the queue that is not free */
int get_free_thread(int thread){
    int tid = thread;
    for(int i = 0 ; i < threads_num ;++i) {
        if (!(load_flags[tid] || store_flags[tid] || halted_flags[tid])) {
            if(tid != thread) cyc_counter += SIM_GetSwitchCycles() + 1;
            return tid;
        }
        if(tid == threads_num-1) tid =0;
        else tid++;
    }
    return -1;
}

/* Update load and store flags arrays
 * Return false when idle
 * Used for CORE_FinegrainedMT function
 */
bool check_another_free_thread_fg(int load_latency, int store_latency ){
    for(int tid = 0 ; tid < threads_num ;++tid) {
        if (load_flags[tid] && ((cyc_counter - load_arr[tid] > load_latency)))
            load_flags[tid] = false; // Update flags if no long busy
    }

    for(int tid = 0 ; tid < threads_num ;++tid) {
        if (store_flags[tid] && (cyc_counter - store_arr[tid] > store_latency))
            store_flags[tid] = false; // Update flags if no long busy
    }

    for(int tid = 0 ; tid < threads_num ;++tid) {
        if (!(load_flags[tid] || store_flags[tid] || halted_flags[tid]))
            return true;
    }
    return false;
}


void CORE_BlockedMT() {
    //initialize variables
    int tid = 0, halt_counter = 0;
    int load_latency = SIM_GetLoadLat();
    int store_latency = SIM_GetStoreLat();
    threads_num = SIM_GetThreadsNum();
    bool idle_flag = false;
    load_arr = new int[threads_num]; // Save the cycle of the last load
    store_arr = new int[threads_num]; // Save the cycle of the last store
    load_flags = new bool[threads_num]; // indicate if thread in load
    store_flags = new bool[threads_num]; // indicate if thread in store
    halted_flags = new bool[threads_num]; // indicate if thread in halted
    line_per_thread = new uint32_t [threads_num]; // Save for each thread the current line
    register_file_per_thread = new tcontext[threads_num]; // Contain the register file for each thread
    dst_value = new int32_t;
    instruction = new Instruction;

    zero_arr(); //initialize all the arrays to be zeros before using.

    while (halt_counter != threads_num){ // Loop will end until all thread will be halted
      if(!check_another_free_thread(load_latency, store_latency)){ // Check if not idle
          cyc_counter++;
          idle_flag = true;
          continue;
      }
        int thread = tid;
        tid = get_free_thread(tid); //Get the thread that is free according to the Blocked MT
        if(idle_flag && thread == tid){
            ++cyc_counter;
        }

        idle_flag = false;
        SIM_MemInstRead(line_per_thread[tid],instruction,tid); // Get the instruction
        line_per_thread[tid]++; // Update the command line

        num_of_instructions++;
        int s1 = instruction -> src1_index;
        int s2 = instruction -> src2_index_imm;
        int dst = instruction -> dst_index;
        int32_t offset = 0;
        switch (instruction->opcode) { // Case for each command
            case CMD_NOP:{
                break;
            }
            case CMD_ADD:{
                register_file_per_thread[tid].reg[dst] = register_file_per_thread[tid].reg[s1] + register_file_per_thread[tid].reg[s2];
                break;
            }

            case CMD_SUB:{
                register_file_per_thread[tid].reg[dst] = register_file_per_thread[tid].reg[s1] - register_file_per_thread[tid].reg[s2];
                break;
            }

            case CMD_ADDI:{
                register_file_per_thread[tid].reg[dst] = register_file_per_thread[tid].reg[s1] + s2;
                break;
            }

            case CMD_SUBI:{
                register_file_per_thread[tid].reg[dst] = register_file_per_thread[tid].reg[s1] - s2;
                break;
            }
            case CMD_LOAD:{
                load_arr[tid] = cyc_counter; // Update the cycle number the command occurred per thread
                load_flags[tid] = true; // Update the flag per thread
                if(instruction ->isSrc2Imm){
                    offset=register_file_per_thread[tid].reg[s1] + s2;
                }

                else{
                    offset=register_file_per_thread[tid].reg[s1] + register_file_per_thread[tid].reg[s2];
                }

                SIM_MemDataRead((uint32_t)offset, dst_value); // Read from the memory
                register_file_per_thread[tid].reg[dst] = *dst_value; // Update the register file of the particular thread
                if(check_another_free_thread( load_latency, store_latency)){ // Update thread if needed
                  tid = get_free_thread(tid);
                    continue;
                }
                break;
            }

            case CMD_STORE:{
                store_arr[tid] = cyc_counter; // Update the cycle number the command occurred per thread
                store_flags[tid] = true; // Update the flag per thread
                if(instruction -> isSrc2Imm){
                    offset=register_file_per_thread[tid].reg[dst] + s2;
                }
                else{
                    offset=register_file_per_thread[tid].reg[dst] + register_file_per_thread[tid].reg[s2];
                }

                SIM_MemDataWrite(offset, register_file_per_thread[tid].reg[s1]); // write to the register file from the memory
                if(check_another_free_thread( load_latency, store_latency)){ //Update thread if needed
                    tid = get_free_thread(tid);
                    continue;
                }
                break;
            }

            default: { //HALT
                halt_counter++;
                halted_flags[tid] = true; //update the halted array
                if( halt_counter == threads_num) break; // When all thread are halted we finish
                if(check_another_free_thread( load_latency, store_latency)){
                    tid = get_free_thread(tid);
                    continue;
                }
            }
        }
        cyc_counter++;
    }
    cyc_number_blocked = cyc_counter;
}

void CORE_FinegrainedMT() {

//initialize variables
    num_of_instructions = 0;
    int tid = 0;
    int halt_counter = 0;
    int load_latency = SIM_GetLoadLat();
    int store_latency = SIM_GetStoreLat();
    threads_num = SIM_GetThreadsNum();
    int switch_cyc = SIM_GetSwitchCycles();
    int thread = 0;
    cyc_counter = 0;

    zero_arr(); //initialize all the arrays to be zeros before using.

   while (halt_counter != threads_num) { // Loop will end until all thread will be halted
        if (!check_another_free_thread_fg(load_latency, store_latency)) { // Check if not idle
            if(!halted_flags[tid]){
                cyc_counter++;
            }
            else { // Get the next thread in the Queue the is not halted when on Idle
                tid = get_next_not_halted_thread(tid);
            }
            continue;
        }

       thread = tid;
       tid = get_free_thread(tid);
       if (thread != tid) {
           cyc_counter -= (switch_cyc + 1); // We do this operation because we use the function of the Blocked MT,therefore we need to decrease switch cycles
       }

        SIM_MemInstRead(line_per_thread[tid], instruction, tid); //Get the instruction
        line_per_thread[tid] += 1; // Update the command line

    /* //for debug
        cout << "cycle: " << cyc_counter << endl;
        cout << "curr_tid: " << tid << endl;
        cout << "opcode: "<<instruction->opcode <<endl;
        cout<<""<<endl;
    */
        num_of_instructions++;
        int s1 = instruction->src1_index;
        int s2 = instruction->src2_index_imm;
        int dst = instruction->dst_index;
        int32_t offset = 0;
        switch (instruction->opcode) { // Case for each command
            case CMD_NOP: {
                break;
            }

            case CMD_ADD: {
                register_file_per_thread[tid].reg[dst] =
                        register_file_per_thread[tid].reg[s1] + register_file_per_thread[tid].reg[s2];
                break;
            }

            case CMD_SUB: {
                register_file_per_thread[tid].reg[dst] =
                        register_file_per_thread[tid].reg[s1] - register_file_per_thread[tid].reg[s2];
                break;
            }

            case CMD_ADDI: {
                register_file_per_thread[tid].reg[dst] = register_file_per_thread[tid].reg[s1] + s2;
                break;
            }

            case CMD_SUBI: {
                register_file_per_thread[tid].reg[dst] = register_file_per_thread[tid].reg[s1] - s2;
                break;
            }

            case CMD_LOAD: {
                load_arr[tid] = cyc_counter; // Update the cycle number the command occurred per thread
                load_flags[tid] = true; // Update the flag per thread
                if (instruction->isSrc2Imm) {
                    offset = register_file_per_thread[tid].reg[s1] + s2;
                }

                else {
                    offset = register_file_per_thread[tid].reg[s1] + register_file_per_thread[tid].reg[s2];
                }

                SIM_MemDataRead((uint32_t)offset, dst_value); // Read from the memory
                register_file_per_thread[tid].reg[dst] = *dst_value; // Update the register file of the particular thread
                break;
            }

            case CMD_STORE: {
                store_arr[tid] = cyc_counter; // Update the cycle number the command occurred per thread
                store_flags[tid] = true; // Update the flag per thread
                if (instruction->isSrc2Imm) {
                    offset = register_file_per_thread[tid].reg[dst] + s2;
                }

                else {
                    offset = register_file_per_thread[tid].reg[dst] + register_file_per_thread[tid].reg[s2];
                }

                SIM_MemDataWrite(offset, register_file_per_thread[tid].reg[s1]); // write to the register file from the memory
                break;
            }

            default: { //HALT
                halt_counter++;
                halted_flags[tid] = true;
                if (halt_counter == threads_num) {
                    continue; // End the loop
                }
            }
        }

        tid = get_next_not_halted_thread(tid); // Update to the next thread in queue that is not halted
        cyc_counter++;
    }
    cyc_number_fg = cyc_counter+1;

   //Free memory
    delete[] load_arr;
    delete[] store_arr;
    delete[] load_flags;
    delete[] store_flags;
    delete[] halted_flags;
    delete[] line_per_thread;
    delete dst_value;
    delete instruction;
}

double CORE_BlockedMT_CPI(){
	return (double)cyc_number_blocked/num_of_instructions;
}

double CORE_FinegrainedMT_CPI(){
    delete [] register_file_per_thread; //Free the register file we create
	return (double)cyc_number_fg/num_of_instructions;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
    for(int i = 0 ; i < REGS_COUNT ; ++i){
        context[threadid].reg[i] = register_file_per_thread[threadid].reg[i];
    }
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
    for(int i = 0 ; i < REGS_COUNT ; ++i){
        context[threadid].reg[i] = register_file_per_thread[threadid].reg[i];
    }
}
