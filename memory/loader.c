
// This code file is fully implement by Rachana Gupta

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "simos.h"

// #define progError -1 in simos.h

// definitions included from paging.c
#define opcodeShift 24
#define operandMask 0x00ffffff
#define diskPage -2
// from cpu.c
#define OPifgo 5

FILE *fPtr;

/**
Similar to real systems, simOS implements a loader (loader.c) to load submitted
programs into memory and into the swap space. loader.c is called by
submit_process in process.c
**/

/* if a program error is detected, the loading procedure terminates and the
 * loader rurns “progError”. Otherwise, load_process_to_swap rurns
 * progNormal*/

//---------------------------------------------------------------------------//
 // Function required by simos.h are following function
 // 1:  int load_process (int pid, char *fname);
          // This is the major function in loader.c
          // after program submission, load_process loads the program to both
          // memory and swap space (this function is called by process.c)
// 2:  void load_idle_process ();
  // load an idle process so that CPU will always having something to do

// return status of loader, whether the program being loaded is correct
// #define progError -1
// #define progNormal 1

//---------------------------------------------------------------------------//

int load_pages_to_memory (int pid, int numpage);
int load_process_to_swap (int pid, char *fname);

//---------------------------------------------------------------------------//

//----------------------//
//  Loader Function
// ---------------------//

int load_process (int pid, char *fname) // Rachana Gupta
{
  // gives back loaded pages
  init_process_pagetable(pid);
  int rPages;
  rPages = load_process_to_swap(pid, fname);
  int j = 0;
  while (j < rPages) {
    PCB[pid]->PTptr[j] = diskPage;
    j++;
  }

  if (rPages > 0) {
    load_pages_to_memory(pid, loadPpages);
  }
  return (rPages);
} // end laod process



void load_idle_process () // Rachana Gupta
{

  int page;
  int frame;
  int instruct;
  int operand;
  int data;
  int opcode;

  init_process_pagetable (idlePid);

  page = 0;
  frame = OSpages - 1;
  //update pagetable and new frame info
  update_process_pagetable (idlePid, page, frame);
  update_frame_info (frame, idlePid, page);

  //load data for idle process
  opcode = OPifgo;
  operand = 0;
  instruct = (opcode << opcodeShift) | operand;
  direct_put_instruction (frame, 0, instruct);
  direct_put_instruction (frame, 1, instruct);
  direct_put_data (frame, 2, 1);
  PCB[idlePid]->dataOffset=2;
} // end laod idie process

//-------------------------------------------------------------------------------//

//----------------------//
//  Loader Helper Function
// ---------------------//

// 1: this function called by swap.c

int load_instruction (mType *buf, int page, int offset) // Rachana Gupta
{
  // writing to one or more buffers

  // int operand;
  // int cpu_ocode;
  // fscanf(fPtr, "%d %d\n", &cpu_ocode, &operand);
  // fscanf(fPtr, "%d %d\n", &cpu_ocode, &operand);
  //   //printf("Load the instruction: %d, %d\n", cpu_ocode, operand);
  // cpu_ocode = cpu_ocode << opcodeS

  int address = (page * pageSize) + offset;
  Memory[address].mInstr = buf->mInstr;

} // end load instruct


// 2 : This function called by swap.c
// load data to buffer
int load_data (mType *buf, int page, int offset) // Rachana Gupta
{

// // loads data to buffer
//  int progData;
//  fscanf(fPtr, "%d\n", &progData);
//  // printf("Load the data: %d\n", progData);

  int address = (page * pageSize) + offset;
  Memory[address].mData = buf->mData;
} // end load data

// 3: This function is heler fucntion for load process
// purpose : load program to swap space, returns the #pages loaded

int load_process_to_swap (int pid, char *fname) // Rachana Gupta
{
 int size;
  int nInst;
  int numdata;
  int check;
  int i;
  int offset;
  int p;
  int opcode, operand;
  float data;
  mType *buf = (mType *) malloc (pageSize*sizeof(mType));
  //mType *buf2 = (mType *) malloc (pageSize*sizeof(mType));
  int frame;
  int tInstr, tOpcode, tOperand;
  unsigned *temp = (unsigned *) malloc (pageSize*sizeof(unsigned));


  init_process_pagetable(pid);

  // open file to
  // read from file fname
  fPtr = fopen (fname, "r");

  //  Error case 1:
  if (fPtr == NULL)
  {
    //empty filename
    printf("Program name is incorrect: %s!\n", fname);
    return progError;
  }
  check = fscanf (fPtr, "%d %d %d\n", &size, &nInst, &numdata);
    //  Error case 2:
  if (check < 3)   // less than 3 parameters
  {
    printf ("Program needs three paramenters, submit has failed: needs %d more parameters\n", 3-check);
    return progError;
  }

  p=0;
  offset=0; // offset

  PCB[pid]->dataOffset = nInst;


  mType *buf2 = (mType *) malloc (pageSize*sizeof(mType));

// start reading data from file
// loop throgh the total number  of instrunction
// and load data into the buffer
  for (i=0; i< nInst; i++)
  {
    fscanf (fPtr, "%d %d\n", &opcode, &operand);

	opcode = opcode << opcodeShift;
  operand = operand & operandMask;


  // and load data into the buffer
	buf[offset].mInstr = opcode | operand;

	temp[offset] = (unsigned)buf[offset].mInstr;

	offset++;

	if (offset==pageSize)
  {
		//write_swap_page (pid, p, buf);
		insert_swapQ(pid,p,temp,actWrite,Nothing);
    //write back to disk space
		update_process_pagetable (pid, p, -2);
		offset=0;
    p++;

	}  // end if

} // end for

// id there is offset left
  while (offset<pageSize)
  {
    buf[offset].mInstr=0;
    temp[offset] = 0;
    offset++;
  } // while

  //  write the program into swap space by inserting it to swapQ

  //write_swap_page (pid, p, buf);
  insert_swapQ(pid,p,temp,actWrite,Nothing);

  // update the process page table
  // when the page is not empty
  // write back to disk space
  update_process_pagetable (pid, p, -2);

  offset=0;

  p++;
  // data offset is the address
  PCB[pid]->dataOffset = p*pageSize + offset;


// loop throught the number of data
  for (i=0; i<numdata; i++)
  {
    fscanf (fPtr, "%f\n", &buf[offset].mData);

	temp[offset] = (unsigned)buf[offset].mInstr;
	offset++;
	if (offset==pageSize)
  {
		//write_swap_page (pid, p, buf);
		insert_swapQ(pid,p,temp,actWrite,Nothing);
    // write back to disk space
		update_process_pagetable (pid, p, -2);
		offset=0;
    p++;
	} // end if
} // end for

   //write_swap_page
   insert_swapQ(pid,p,temp,actWrite,Nothing);
   // write back to disk space
   update_process_pagetable (pid, p, -2);
   dump_process_pagetable(pid);

   // if not return error,
   // return 1
   return 1; //p;
} // end load process to swap


// 3 : this function called by load instruction helper function
int load_pages_to_memory (int pid, int numpage) // Rachana Gupta
{
  // swap.c puts the process in ready queue waiting after the last write
  unsigned *temp = (unsigned *) malloc (pageSize*sizeof(unsigned));
  int i;

  int *frameNum = (int *) malloc (numpage*sizeof(int));

  for (i=0;i<=numpage;i++)
  {
    // get free frame
	   frameNum[i]=get_free_frame();
    // update frame infor after get free frame
	   update_frame_info(frameNum[i], pid, i);
     //load pages from process pid to memory
	   insert_swapQ(pid,i, temp, actRead, Nothing);

	   update_process_pagetable(pid, i, -3);
   }
   for (i=0;i<=numpage;i++)
   {
	   update_process_pagetable(pid, i, frameNum[i]);
	   update_frame_info(frameNum[i], pid, i);
   }

} // end load page to memory
