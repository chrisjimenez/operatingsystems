//Created by Professor Goldberg
//Edited by Christopher Jimenez
//Project 1

#include <stdio.h>
#include <stdlib.h>

//header files to be included
#include "hardware.h"
#include "drivers.h"
#include "kernel.h"


/* You may use the definitions below, if you are so inclined, to
   define the process table entries. Feel free to use your own
   definitions, though. */

/* A quantum is 40 ms */
#define QUANTUM 40

typedef enum { RUNNING, READY, BLOCKED , UNINITIALIZED } PROCESS_STATE;

typedef struct process_table_entry {
  PROCESS_STATE state;
  int CPU_time_used;
  int quantum_start_time;
} PROCESS_TABLE_ENTRY;

extern PROCESS_TABLE_ENTRY process_table[];
PROCESS_TABLE_ENTRY process_table[MAX_NUMBER_OF_PROCESSES];


/* You may use this code for maintaining a queue of
   ready processes. It's simply a linked list structure
   with each element containing the PID of a ready process */    

typedef struct ready_queue_elt {
  struct ready_queue_elt *next;
  PID_type pid;
} READY_QUEUE_ELT;
READY_QUEUE_ELT *ready_queue_head = NULL;  /* head of the event queue */
READY_QUEUE_ELT *ready_queue_tail = NULL;


int active_processes = 0;  /* count to keep track of active processes   */

//////////////////////////////////////////////////////////////////////////////////
//places process id at end of ready queue
void queue_ready_process(PID_type pid){
  READY_QUEUE_ELT *p = (READY_QUEUE_ELT *) malloc(sizeof(READY_QUEUE_ELT));
  p->pid = pid;
  p->next = NULL;
  if (ready_queue_tail == NULL) 
    if (ready_queue_head == NULL) {
      ready_queue_head = ready_queue_tail = p;
      p->next = NULL;
    }
    else {
      printf("Error: ready queue tail is NULL but ready_queue_head is not\n");
      exit(1);
    }
  else {
    ready_queue_tail->next = p;
    ready_queue_tail = p;
  }
}

/////////////////////////////////////////////////////////////////////////////////
//removes and returns PID from front of ready queue
PID_type dequeue_ready_process(){
  if (ready_queue_head == NULL)
    if (ready_queue_tail == NULL)
      return IDLE_PROCESS;        // indicates no active process is ready
    else {
      printf("Error: ready_queue_head is NULL but ready_queue_tail is not\n");
      exit(1);
    }
  else {      
    READY_QUEUE_ELT *p = ready_queue_head;
    ready_queue_head = ready_queue_head->next;
    if (ready_queue_head == NULL)
      ready_queue_tail = NULL;
    return p->pid;
  }
}

/////////////////////////////////////////////////////////////////////////////////
//initializes process table
void init_process_table(){
  printf("Initializing Process Table...\n");

  for(int i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
    if(i == 0){
      process_table[i].state = RUNNING;
      process_table[i].quantum_start_time = clock;
      process_table[i].CPU_time_used = 0;
            
      active_processes++;        
    }else{
      process_table[i].state = UNINITIALIZED;
      process_table[i].CPU_time_used = 0;
      process_table[i].quantum_start_time = 0;
    }
  }
}

///////////////////////////////////////////////////////
void get_next_process(){    
  PID_type next_process = dequeue_ready_process();
    
  //if the next process is idle..... 
  if(next_process == IDLE_PROCESS){
    printf("Time: %d: Processor is idle\n", clock);
    current_pid = IDLE_PROCESS;
  } else {  //if the enxt process is NOT idle....
    current_pid = next_process;

    //set current_pid quantum_start-time to clock
    process_table[current_pid].quantum_start_time = clock;
    //set current process to RUNNING...
    process_table[current_pid].state = RUNNING;
        
    printf("Time: %d: Process %d runs\n", clock, current_pid);
  }
}

/////////////////////////////////////////////////////////////////////////////////
//  HANDLERS
void trap_handler(){
  int trap_type = R1;

  if(trap_type == DISK_READ){
    int size = R2;
    //call disk_read_req from drivers.h
    disk_read_req(current_pid, size);

    //compute cpu time used...
    //set the current process to blocked
    process_table[current_pid].state = BLOCKED;

    //go to next process..
    get_next_process();

  }else if(trap_type == DISK_WRITE){
    disk_write_req(current_pid);
    //no need to go to the next process....

  }else if(trap_type == KEYBOARD_READ){
    //set the current process to blocked
    process_table[current_pid].state = BLOCKED;

    //call keyboard_read_req from drivers.h
    keyboard_read_req(current_pid);

    //go to next process..
    get_next_process();
    
  }else if(trap_type == FORK_PROGRAM){
    int forked_process = R2;

    //incrememnt number of active processes.
    active_processes++;

    //set forked processes to READY...
    process_table[forked_process].state = READY; 

    //call queue_ready_process from drivers.h
    queue_ready_process(forked_process);
    
    printf("Time: %d: Creating process entry for pid %d\n", clock, forked_process);
    
  }else{ //(trap_type == END_PROGRAM)
    int ending_process = R2;

    printf("Time: %d: Process %d exits. Total CPU time = %d\n", clock, ending_process, process_table[ending_process].CPU_time_used);
    process_table[ending_process].state = UNINITIALIZED;
            
    active_processes--;
    
    //Check if there are anymore processes....
    if(active_processes == 0){
      printf("-- No more processes to execute --\n");
      exit(0);
    }
    //go to next process.. 
    get_next_process();
  }
}

void clock_interrupt_handler(){
  //if the current process is NOT idle...
  if(current_pid != IDLE_PROCESS){
    if( (clock - process_table[current_pid].quantum_start_time) >= QUANTUM){
      //current process is read
      process_table[current_pid].state = READY;
      queue_ready_process(current_pid);

      //go to next process...
      get_next_process();
    }
    process_table[current_pid].CPU_time_used += CLOCK_INTERRUPT_PERIOD;   
  }
}

void disk_interrupt_handler(){
  int disk_interrupt_pid = R1;

  printf("Time: %d: handled DISK INTERRUPT for pid %d\n", clock, disk_interrupt_pid);

  //set process to ready...
  process_table[disk_interrupt_pid].state = READY;

  //Q the process
  queue_ready_process(disk_interrupt_pid);
    
  //If the current process is idle, then get next process
  if(current_pid == IDLE_PROCESS) {
    get_next_process();
  }
}

void keyboard_interrupt_handler(){
  int keyboard_interrupt_pid = R1;

  printf("Time: %d: handled KEYBOARD INTERRUPT for pid %d\n", clock, keyboard_interrupt_pid);

  //set process to ready...
  process_table[keyboard_interrupt_pid].state = READY;

  //Q the process
  queue_ready_process(keyboard_interrupt_pid);
    
  //If the current process is idle, then get next process
  if(current_pid == IDLE_PROCESS) {
    get_next_process();
  }
}

/////////////////////////////////////////////////////////////////////////////////
//Sets up the interrupt table by installign interrupt handlers...
void setup_interrupt_table(){
  printf("Setting up Interrupt Table...\n");

  INTERRUPT_TABLE[TRAP] = trap_handler;
  INTERRUPT_TABLE[CLOCK_INTERRUPT] = clock_interrupt_handler;
  INTERRUPT_TABLE[DISK_INTERRUPT] = disk_interrupt_handler;
  INTERRUPT_TABLE[KEYBOARD_INTERRUPT] = keyboard_interrupt_handler;
}


////////////////////////////////////////////////////////////////////////////////
//This procedure is automatically called when the (simulated) machine boots up
void initialize_kernel(){
    printf("Initializing Kernel...\n");
    
    //initialize process_table array
    init_process_table();

    //Setup the interrupt handlers to INTERRUPT_TABLE
    setup_interrupt_table();  
}
