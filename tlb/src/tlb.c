//Created by Professor Goldberg
//Edited by Christopher Jimenez
//Project 2

#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "tlb.h"
#include "cpu.h"
#include "mmu.h"

/* This is some of the code that I wrote. You may use any of this code
   you like, but you certainly don't have to.
*/

/* I defined the TLB as an array of entries,
   each containing the following:
   Valid bit: 1 bit
   Virtual Page: 20 bits
   Modified bit: 1 bit
   Reference bit: 1 bit
   Page Frame: 20 bits (of which only 18 are meaningful given 1GB RAM)
*/

//You can use a struct to get a two-word entry.
typedef struct {
  unsigned int vbit_and_vpage;  // 32 bits containing the valid bit and the 20bit
                                // virtual page number.
  unsigned int mr_pframe;       // 32 bits containing the modified bit, reference bit,
                                // and 20-bit page frame number
} TLB_ENTRY;


// This is the actual TLB array. It should be dynamically allocated
// to the right size, depending on the num_tlb_entries value 
// assigned when the simulation started running.

TLB_ENTRY *tlb;  

// This is the TLB size (number of TLB entries) chosen by the 
// user. 

unsigned int num_tlb_entries;

  //Since the TLB size is a power of 2, I recommend setting a
  //mask to perform the MOD operation (which you will need to do
  //for your TLB entry evicition algorithm, see below).
unsigned int mod_tlb_entries_mask;

//this must be set to TRUE when there is a tlb miss, FALSE otherwise.
BOOL tlb_miss; 


//If you choose to use the same representation of a TLB
//entry that I did, then these are masks that can be used to 
//select the various fields of a TLB entry.

#define VBIT_MASK   0x80000000  //VBIT is leftmost bit of first word
#define VPAGE_MASK  0x000FFFFF            //lowest 20 bits of first word
#define RBIT_MASK   0x80000000  //RIT is leftmost bit of second word
#define MBIT_MASK   0x40000000  //MBIT is second leftmost bit of second word
#define PFRAME_MASK 0x000FFFFF            //lowest 20 bits of second word


// Initialize the TLB (called by the mmu)
void tlb_initialize(){
  //Here's how you can allocate a TLB of the right size
  tlb = (TLB_ENTRY *) malloc(num_tlb_entries * sizeof(TLB_ENTRY));

  //This is the mask to perform a MOD operation (see above)
  mod_tlb_entries_mask = num_tlb_entries - 1;  

  //call tlb_clear_all()...
  tlb_clear_all();
}

///////////////////////////////////////////////////////////////////////////////
// This clears out the entire TLB, by clearing the valid bit for every entry.
void tlb_clear_all() {
  for(int i = 0; i < num_tlb_entries;i++){
    tlb[i].vbit_and_vpage = 0; 
  }
}

//////////////////////////////////////////////////////////////////////////
//clears all the R bits in the TLB
void tlb_clear_all_R_bits() {
  //tilde is the NOT operator...
  for (int i = 0; i < num_tlb_entries; i++){
    tlb[i].mr_pframe = (tlb[i].mr_pframe & ~RBIT_MASK);
  }
}

/////////////////////////////////////////////////////////////////////
// This clears out the entry in the TLB for the specified
// virtual page, by clearing the valid bit for that entry.
void tlb_clear_entry(VPAGE_NUMBER vpage) {
  for ( int i = 0; i < num_tlb_entries; i++) {
    if( (tlb[i].vbit_and_vpage & VPAGE_MASK) == vpage){
      tlb[i].vbit_and_vpage = (tlb[i].vbit_and_vpage & ~VBIT_MASK);
    } 
  }
}

/////////////////////////////////////////////////////////////////////////
// Returns a page frame number if there is a TLB hit. If there is a TLB
// miss, then it sets tlb_miss (see above) to TRUE.  It sets the R
// bit of the entry and, if the specified operation is a STORE,
// sets the M bit.
PAGEFRAME_NUMBER tlb_lookup(VPAGE_NUMBER vpage, OPERATION op){
  int tlb_hit = FALSE;
    
  PAGEFRAME_NUMBER result = 0;
  
  for (int i = 0; i < num_tlb_entries; ++i){
    //check if vbit is valid
    if( (tlb[i].vbit_and_vpage & VBIT_MASK) != 0){
      if( (tlb[i].vbit_and_vpage & VPAGE_MASK) == vpage){
        tlb[i].mr_pframe = (tlb[i].mr_pframe | RBIT_MASK);
        
        tlb_hit = TRUE;
        tlb_miss = FALSE;
        
        if( op == STORE ){tlb[i].mr_pframe = (tlb[i].mr_pframe | MBIT_MASK);}
        
        //update result...
        result = (tlb[i].mr_pframe & PFRAME_MASK);
      }
    }
  }
  //Finally, check if entry was found  
  if(!tlb_hit) tlb_miss = TRUE;
   
   //return result... 
  return result;
}



// Uses an NRU clock algorithm, where the first entry with
// either a cleared valid bit or cleared R bit is chosen.

int clock_hand = 0;  // points to next TLB entry to consider evicting

//////////////////////////////////////////////////////////////////////////
// 
void tlb_insert(VPAGE_NUMBER new_vpage, PAGEFRAME_NUMBER new_pframe,
  BOOL new_mbit, BOOL new_rbit){

  // Starting at the clock_hand'th entry, find first entry to
  // evict with either valid bit  = 0 or the R bit = 0. If there
  // is no such entry, then just evict the entry pointed to by
  // the clock hand.
  int entry_found = FALSE;
  int entry = clock_hand;

  //loop until the entry is found..
  while(!entry_found){
    if( ((tlb[entry].vbit_and_vpage & VBIT_MASK) == 0) || ((tlb[entry].mr_pframe & RBIT_MASK) == 0) ){
      entry_found = TRUE;
    }else{//update entry...
      entry = (entry + 1) % num_tlb_entries;  // increment i using mod
    }
  }


  // Then, if the entry to evict has a valid bit = 1,
  // write the M and R bits of the of entry back to the M and R
  // bitmaps, respectively, in the MMU (see mmu_modify_rbit_bitmap, etc.
  // in mmu.h)
  if( (tlb[entry].vbit_and_vpage & VBIT_MASK) != 0) {
    mmu_modify_rbit_bitmap((tlb[entry].mr_pframe & PFRAME_MASK) , (tlb[entry].mr_pframe & RBIT_MASK));
    mmu_modify_mbit_bitmap((tlb[entry].mr_pframe & PFRAME_MASK) , (tlb[entry].mr_pframe & MBIT_MASK));
  }


  // Then, insert the new vpage, pageframe, M bit, and R bit into the
  // TLB entry that was just found (and possibly evicted).
  tlb[entry].vbit_and_vpage = new_vpage;
  tlb[entry].mr_pframe = new_pframe;
  tlb[entry].vbit_and_vpage = (tlb[entry].vbit_and_vpage | VBIT_MASK);

  //Ccondition if the new_rbit is true...
  if( new_rbit == TRUE) {
    tlb[entry].mr_pframe = (tlb[entry].mr_pframe | RBIT_MASK);
  } else {
    tlb[entry].mr_pframe = (tlb[entry].mr_pframe & ~RBIT_MASK);
  }

  //Ccondition if the new_mbit is true...
  if( new_mbit == TRUE) {
    tlb[entry].mr_pframe = (tlb[entry].mr_pframe | MBIT_MASK);
  } else {
    tlb[entry].mr_pframe = (tlb[entry].mr_pframe & ~MBIT_MASK);
  }

  // Finally, set clock_hand to point to the next entry after the
  // entry found above. 
  clock_hand = (entry + 1) % num_tlb_entries;
}

///////////////////////////////////////////////////////////////////////
//Writes the M  & R bits in the each valid TLB
//entry back to the M & R MMU bitmaps.
void tlb_write_back(){
  for ( int i = 0; i < num_tlb_entries; i++ ) {
    if( (tlb[i].vbit_and_vpage & VBIT_MASK) != 0){
      mmu_modify_rbit_bitmap((tlb[i].mr_pframe & PFRAME_MASK) , (tlb[i].mr_pframe & RBIT_MASK));
      mmu_modify_mbit_bitmap((tlb[i].mr_pframe & PFRAME_MASK) , (tlb[i].mr_pframe & MBIT_MASK));
    }
  }
}

