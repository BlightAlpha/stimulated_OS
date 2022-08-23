#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "simos.h"

// -----------------------------------------------------------------------------//
// paging.c
// implement by
// Victor Chaing as simple paging Manager and,
// Surapa Phrompha as Virtual paging manager

// -----------------------------------------------------------------------------//
// According to simos.h
// paging.c requre to implemt the following functions

// page table management functions for managing page table of each process

// 1. void init_process_pagetable (int pid); (Victor Chaing)
// 2. void update_process_pagetable (int pid, int page, int frame); (Surapa Phrompha)
// 3. void free_process_memory (int pid); (Surapa Phrompha)
// 4. void dump_process_pagetable (FILE *outf, int pid); (Victor Chaing)
// 5. void dump_process_memory (FILE *outf, int pid); (Victor Chaing)
//
// 6. void update_frame_info (int findex, int pid, int page); (Surapa Phrompha)
// 7. void swap_in_page (int pidin, int pagein, int finishact);
//   // loader.c and memory.c uses the above function
//   // pagein is the page to be swapped in, going by page offset
//   // finishact is defined in swap.c and only used when calling insert_swapQ

// 9. void dump_memory (FILE *outf);           (Victor Chaing)
// 10. void dump_memoryframe_info (FILE *outf); (Victor Chaing)
//
//   // interrupt handling functions for memory, called by cpu.c
// 11. void page_fault_handler ();
// 12. void memory_agescan ();
//
// 13. void initialize_mframe_manager ();   // called by system.c (Victor Chaing)
// 14. void initialize_agescan ();   // called by system.c
//
// 15. int calculate_memory_address (unsigned offset, int rwflag);  // memory.c (Victor Chaing)


//---------------------------------------------------------------------------------------------//

//Global variables and struct needed for paging.c

//mType *Memory;   // The physical memory
typedef unsigned ageType;
typedef struct
{
    int page, pid, next, prev;
    char free, dirty, pin;
    ageType age;
}FrameStruct;


FrameStruct* physicalFrame;
int frameHead, frameTail;
unsigned pageOSMask;
int pageNumShift;

unsigned pageoffsetMask;
int pagenumShift;

//all necessary defines
//------------------------------------------------------------------------//
//
//---------------------------------//
// Global Variable for the frames
//---------------------------------//
#define DIRTY_FRAME 1
#define FREE_FRAME 1
#define PIN_FRAME 1
#define CLEAN_FRAME 0
#define USED_FRAME 0
#define NONPIN_FRAME 0

//-----------------------------------------------------------------------//


#define NULLINDEX -1
#define NULLPAGE -1               // for null page status
#define DISKPAGE -2               // for dispage status
#define PENDPAGE -3               // for pending page status


#define AGEZERO 0x00000000        // starting age
#define AGEMAX 0x80000000         // max age
#define SHIFT_CODE 24
#define MASK_OPERAND 0x00ffffff
#define FLAG_READ 1
#define FLAG_WRITE 2

// ---------------------------------------------------------------------------------


// The function include the funtion requre by simos.h

void init_process_pagetable (int pid); //(Victor Chaing)
void update_process_pagetable (int pid, int page, int frame); //(Surapa Phrompha)
int free_process_memory (int pid); //(Surapa Phrompha)
void dump_process_pagetable(int pid); //(Victor Chaing)
void dump_process_memory(int pid); //(Victor Chaing)

void  update_frame_info (int frame_index, int pid, int page); //(Surapa Phrompha)

void dump_memory();     //(Victor Chaing)
void dump_memoryframe_info(); //(Victor Chaing)

void page_fault_handler (); //(Surapa Phrompha)
void memory_agescan ();    //(Surapa Phrompha)

void initialize_mframe_manager (); //(Victor Chaing)
int calculate_memory_address(unsigned offset, int flag); //(Victor Chaing)


//-----------------------------------------------------------------------------------//

// -----------------//
// Helper Function
// -----------------//


void addto_freeMemoryFrame (int frame_index, int status); //(Surapa Phrompha)
int find_allocated_memory(int pid, int page) ; //(Surapa Phrompha)
int check_for_pending_page(int pid, int page); //(Surapa Phrompha)
int find_next_page(int pid, int page);    //(Surapa Phrompha)
int find_previous_page(int pid, int page);    //(Surapa Phrompha)
void display_frame(int index);  //(Victor Chaing)
void display1FrameInfo(int index); //(Victor Chaing)
void initialize_memory(); //(Victor Chaing)
void initialize_memory_manager (); //(Victor Chaing)
void display_pagefault(int findex); //(Surapa Phrompha)
int get_free_frame (); //(Surapa Phrompha)
int select_agest_frame (); //(Surapa Phrompha)



//----------------------------------------------------------------------------------------//
void direct_put_instruction (int findex, int offset, int instr);
void direct_put_data (int findex, int offset, mdType data);
void initialize_physical_memory ();

//---------------------------------------------------------------------------------------------//

//-------------------------------------//
// Page Table Management Functions
//-------------------------------------//

//for managing page table of each process


// implement by Victor Chaing and Surapa Phrompha


// purpose : to initailise page Table
// Note :  maxPpages: max #pages for each process (simos.h)
//#define addrSize 4   // each memory address is of size 4 bytes (simos.h)
void init_process_pagetable (int pid) // Surapa Phrompha
{
  int page;
  PCB[pid]->PTptr = (int *) malloc (addrSize*maxPpages);
  for (page=0; page<maxPpages; page++)
  {
    PCB[pid]->PTptr[page] = NULLPAGE;
  }
  PCB[pid]->PC=0;
}

// purpose : to update the page Table
// update by pointing to frame memory, disk, or null
void update_process_pagetable (int pid, int page, int frame) // Surapa Phrompha
{
  PCB[pid]->PTptr[page] = frame;
}


// purpose : to free memory to terminate the process
// Note :
// 1 : int *PTptr = page table ptr (simoes.h)
// 2 : mInstr = type definition for memory content (simoes.h)
// int pageSize =  sizes related to memory and memory management (simoes.h)
int free_process_memory (int pid)  // Surapa Phrompha
{
  int page;
  int j;
  for (page=0; page<maxPpages; page++)
  {
      if (PCB[pid]->PTptr[page] != NULLPAGE)
      {
          for(j = PCB[pid]->PTptr[page] * pageSize; j < (PCB[pid]->PTptr[page] + 1) * pageSize; j++)
          {
            printf("--------------------------- \n");
            printf("Mem: 0x%032x, \n", j);
            printf(" Data: 0x%016x, \n",Memory[j]);
            printf(" %f\n",Memory[j].mData);
            printf("--------------------------- \n");
            Memory[j].mInstr=0;
          }

          addto_freeMemoryFrame(PCB[pid]->PTptr[page], NULLPAGE);
      }
  }

}

//purspose : function dump_process_pagetable
void dump_process_pagetable(int pid)  // Victor Chaing
{
    //printing out the page and frame based upon location
    for (int i = 0; i < maxPpages; i++)
    {
        if (PCB[pid]->PTptr[i] >= 0) {
           printf("--------------------------- \n");
            printf("Page: %d, \n", i);
            printf("Frame: %d \n", PCB[pid]->PTptr[i]);
            printf("\n");
            printf("--------------------------- \n");
        }
        else if (PCB[pid]->PTptr[i] == DISKPAGE) {
            printf("--------------------------- \n");
            printf("Page %d is on disk \n", i);
            printf("\n");
            printf("--------------------------- \n");
        }
        else if (PCB[pid]->PTptr[i] == PENDPAGE) {
            printf("--------------------------- \n");
            printf("Page %d is being loaded into memory \n", i);
            printf("\n");
            printf("--------------------------- \n");
        }
        else {
            printf("--------------------------- \n");
            printf("Page %d DNE \n", i);
            printf("\n");
            printf("--------------------------- \n");
        }
    }
}

//function dump_process_memory
void dump_process_memory(int pid) // Victor Chaing
{
    for (int i = 0; i < maxPpages; i++)
    {
        if (PCB[pid]->PTptr[i] != NULLPAGE)
        {
            display_frame(PCB[pid]->PTptr[i]);
        }
    }
  }



// purpose : update the metadata of a frame, base on different case
// Note
// frame_index = physical memery frame
// page = page in page tale
void  update_frame_info (int frame_index, int pid, int page) // Surapa Phrompha
{
  // in the disk page
  if (PCB[pid]->PTptr[page] == DISKPAGE)
  {
      // for Virtual Memory
  }


  //-----------------------------------------------------------------------------//

  // use link list
   // have to perfrom in this order
   // otherwise, we cannot update the frame

  if (physicalFrame[frame_index].prev != NULLINDEX)
  {
    physicalFrame[physicalFrame[frame_index].prev].next = physicalFrame[frame_index].next;
  }

  // the frame is update on this add
  if (physicalFrame[frame_index].next != NULLINDEX)
  {
      physicalFrame[physicalFrame[frame_index].next].prev = physicalFrame[frame_index].prev;
  }


  //----------------------------------------------------------------------------------//

  physicalFrame[frame_index].prev = find_previous_page(pid,page);


  physicalFrame[frame_index].next = find_next_page(pid,page);

  if (physicalFrame[frame_index].prev > NULLINDEX)
  {
    physicalFrame[physicalFrame[frame_index].prev].next = frame_index;

  }

  if (physicalFrame[frame_index].next > NULLINDEX)
  {
    physicalFrame[physicalFrame[frame_index].next].prev = frame_index;
  }

  //------------------------------------------------------------//

  // use max age
  // since the aging policy is used
  physicalFrame[frame_index].age = AGEMAX;
  physicalFrame[frame_index].free = USED_FRAME;
  physicalFrame[frame_index].dirty = CLEAN_FRAME;
  physicalFrame[frame_index].pin = NONPIN_FRAME;

  physicalFrame[frame_index].pid = pid;
  physicalFrame[frame_index].page = page;
}



//function dump_memoryframe_info
  void dump_memoryframe_info () // Victor Chaing
{
int page;
//printing out initial info

printf("------------------------------------------------------------------- \n");
printf ("Memory Frame Metadata\n");
printf ("Memory frame head/tail: %d/%d\n", frameHead, frameTail);
for (page=OSpages; page<numFrames; page++)
{
printf ("Frame %d: ", page);
 display1FrameInfo(page);
}
printf("------------------------------------------------------------------- \n");



int list  = frameHead;

printf("------------------------------------------------------------------- \n");
printf ("Free memory frame\n");
while(list != NULLINDEX)
{
  printf ("%d ", list);
  list = physicalFrame[list].next;
}
printf("\n");
printf("------------------------------------------------------------------- \n");

}

//function dump_memory
void dump_memory()  // Victor Chaing
{
    for (int i = 0; i < numFrames; i++) {
        display_frame(i);
    }
}



// -----------------------------------------------------------------------------------------------------------//

//--------------------------//
// Helper function
//-------------------------//

// Note : purpose to add to free frame
// 1: #define freeFrame 1
// 2: nullIndex = null pointer
// 3: nullPage = page doesn not exist
// 4: clean frame = fields in FrameStruct
// 5 : freeFrame = fields in FrameStruct
void addto_freeMemoryFrame (int frame_index, int status) // Surapa Phrompha
{
  int temp;

    physicalFrame[frame_index].free = FREE_FRAME;


    if (status == NULLPAGE)
  {
        physicalFrame[frame_index].pid = NULLINDEX;
        physicalFrame[frame_index].page = NULLPAGE;
        physicalFrame[frame_index].dirty = CLEAN_FRAME;
    }

    if (physicalFrame[frame_index].prev != NULLINDEX)
    {
        physicalFrame[physicalFrame[frame_index].prev].next = physicalFrame[frame_index].next;
    }



    if (physicalFrame[frame_index].next != NULLINDEX)
  {

        physicalFrame[physicalFrame[frame_index].next].prev = physicalFrame[frame_index].prev;
    }

    if (frameHead == NULLINDEX)
  {
        physicalFrame[frame_index].prev = NULLINDEX;
        physicalFrame[frame_index].next = NULLINDEX;
        frameHead = frame_index;
        frameTail = frame_index;
    }
  else if(frame_index > frameTail)
  {
          physicalFrame[frame_index].prev=frameTail;
          physicalFrame[frame_index].next = NULLINDEX;
          physicalFrame[frameTail].next = frame_index;
          frameTail = frame_index;
      }
      else if (frame_index < frameHead)
    {
          physicalFrame[frame_index].next=frameHead;
          physicalFrame[frame_index].prev = NULLINDEX;
          physicalFrame[frameHead].prev = frame_index;
          frameHead = frame_index;
      }
      else
    {
        temp = frameHead;
        while (frame_index > temp)
    {
      temp = physicalFrame[temp].next;
    }
        physicalFrame[frame_index].prev = physicalFrame[temp].prev;
        physicalFrame[frame_index].next = temp;
        physicalFrame[physicalFrame[temp].prev].next = frame_index;
        physicalFrame[temp].prev = frame_index;
      }  // end else


    if (physicalFrame[frame_index].dirty == CLEAN_FRAME)
    {
        physicalFrame[frame_index].pid = NULLINDEX;
        physicalFrame[frame_index].pid = NULLPAGE;
    }
} // end method


int find_allocated_memory(int pid, int page) //Surapa Phrompha
{
    int allocated_memory;

    // check for next page
    allocated_memory = find_next_page(pid, -1);
    // chekc whether page exist in the memory or not
    while ((physicalFrame[allocated_memory].page!=page) && (allocated_memory!= NULLINDEX))
    {
      // traverse theough the linked list of the memeory
      // until the page is found
      allocated_memory = physicalFrame[allocated_memory].next;
    }
    // retun the idex of page found
    return allocated_memory;
}


// purpose : to find the pending page
// Note :
     // OSpages = #pages for OS, OS occupies the begining of the memory
int check_for_pending_page(int pid, int page) //Surapa Phrompha
{
    int pendingpage = OSpages;


    while (physicalFrame[pendingpage].pid!=pid)
    {
        pendingpage++;

        // if chack until the max pages
        if (pendingpage==maxPpages)
        {
          // it point to the null index
          return NULLINDEX;
        }
    }


    while (physicalFrame[pendingpage].prev != NULLINDEX)
    {
      // get to first frame of pid
      pendingpage = physicalFrame[pendingpage].prev;
    }

    // traverse through the memory frame until found
    // the pending page
    while ((pendingpage!= NULLINDEX) && ((physicalFrame[pendingpage].page!=page) || (physicalFrame[pendingpage].pid!=pid)))
    {

      pendingpage = physicalFrame[pendingpage].next;
    }
    return pendingpage;
}

// purpose : traverse to get the next page
//           that is not null page or disk page
//           (traverse to the left)
int find_next_page(int pid, int page) //Surapa Phrompha
{
    int i = page+1;
    // traverse throght the page table
    while (i!=maxPpages)
    {
      // case 1 :
        if ((PCB[pid]->PTptr[i]==NULLPAGE) ||  (PCB[pid]->PTptr[i]==DISKPAGE))
        {
          // if it is null page or disk pages
          // skip
            i++;
        }
        else if (PCB[pid]->PTptr[i]==PENDPAGE)  // case 2 : it it is pending page
        {
           printf("Pending page\n");
           return check_for_pending_page(pid, i);
        }
        else
        {
             // return the next page if find
            return PCB[pid]->PTptr[i];
        }
    }
    // if reach the max page
    // to next pages
    // reuturn null index
    return NULLINDEX;
}

// purpose : treverse to get the prevous page
//           traverse to the right
int find_previous_page(int pid, int currentPage) //Surapa Phrompha
{
    // from the current page
    int i = currentPage-1;
    // traverse to the right
    while (i!=-1)
    {
        // case 1 : if it is null page or the disk page
        if ((PCB[pid]->PTptr[i]==NULLPAGE) ||  (PCB[pid]->PTptr[i]==DISKPAGE))
        {
          // skip
            i=i-1;
        }
        else if (PCB[pid]->PTptr[i]==PENDPAGE) // case 2: if it is pending page
        {
          printf("Pending page\n");
            return check_for_pending_page(pid, i);
        }
        else
        {
          // reuturn the find_previous_page
          // if it is not the previous two case
            return PCB[pid]->PTptr[i];
        }
    }
    // otherwise return null index
    return NULLINDEX;
}


//----------------------------------------------------------------------------------------------//

//------------------------//
//  Display to the termial
//------------------------//

// use this function to display the information to the terminal
// instead of using FILE *outf

//function display_frame
void display_frame(int index) // (Vitor Chaing)
{
    for (int i = index * pageSize; i < (index + 1) * pageSize; i++)
    {
        printf("-------------------------------------------- \n");
        printf("Memory: 0x%032x \n", i);
        printf("Data: 0x%016x \n", Memory[i]);
        printf("%f \n", Memory[i].mData);
        printf("-------------------------------------------- \n");
    }
}

//function display1FrameInfo
void display1FrameInfo(int index) // (Vitor Chaing)
{
    printf("pid: %d, ", physicalFrame[index].pid);
    printf("page: %d, ", physicalFrame[index].page);
    printf("age: %x, ", physicalFrame[index].age);
    printf("dir: %d, ", physicalFrame[index].dirty);
    printf("free: %d, ", physicalFrame[index].free);
    printf("pin: %d, ", physicalFrame[index].pin);
    printf("next: %d, ", physicalFrame[index].next);
    printf("prev: %d", physicalFrame[index].prev);
    printf("\n");
}


//---------------------------------------------------------------------------------------------//

// --------------------- //
// Simple Paging Manager //
// --------------------- //

// implement by Victor Chiang
//function initialize_memory


void initialize_memory()  // Victor Chiang
{
    physicalFrame = (FrameStruct*)malloc(numFrames * sizeof(FrameStruct));
    Memory = (mType*)malloc(numFrames * pageSize * sizeof(mType));
    pageOSMask = (OSpages - 1) * pageSize - 1;
    pageNumShift = (int)(log((double)(OSpages - 1.0) * pageSize) / log(2.0));
    frameHead = OSpages;
    frameTail = numFrames - 1;
    for (int i = 0; i < OSpages; i++) {
        physicalFrame[i].pid = osPid;
        physicalFrame[i].page = NULLPAGE;
        physicalFrame[i].age = AGEZERO;
        physicalFrame[i].free = USED_FRAME;
        physicalFrame[i].dirty = CLEAN_FRAME;
        physicalFrame[i].pin = PIN_FRAME;
    }
    for (int i = OSpages; i < numFrames; i++) {
        physicalFrame[i].free = USED_FRAME;
        if (i == 0) {
            physicalFrame[i].next = i + 1;
            physicalFrame[i].prev = NULLINDEX;
        }
        else if (i == numFrames - 1) {
            physicalFrame[i].next = NULLINDEX;
            physicalFrame[i].prev = i - 1;
        }
        else {
            physicalFrame[i].next = i + 1;
            physicalFrame[i].prev = i - 1;
        }
        physicalFrame[i].pid = NULLINDEX;
        physicalFrame[i].age = AGEZERO;
        physicalFrame[i].free = FREE_FRAME;
        physicalFrame[i].dirty = CLEAN_FRAME;
        physicalFrame[i].pin = NONPIN_FRAME;
    }
}

//function calculate_memory_address
int calculate_memory_address(unsigned offset, int flag) // Victor Chiang
{
    int index;

    if (offset == 0) {
        index = 0;
    }
    else {
        index = offset / pageSize;
    }

    if ((index >= maxPpages) || (index < 0)) {
        return mError;
    }

    int frame = CPU.PTptr[index];

    if ((frame == NULLPAGE) && (flag == FLAG_READ)) {
        return mError;
    }
    else if (frame == DISKPAGE) {
        if ((flag == FLAG_READ) || (flag == FLAG_WRITE)) {
            set_interrupt(pFaultException);
            return mPFault;
        }
        else {
            return mError;
        }
    }
    else if (frame == PENDPAGE) {
        return mPFault;
    }
    else {
        if ((frame == NULLPAGE) && (flag == FLAG_WRITE)) {
            frame = get_free_frame();
            update_frame_info(frame, CPU.Pid, index);
            update_process_pagetable(CPU.Pid, index, frame);
        }

        int address = (frame * pageSize) + (offset - index * pageSize);

        if (flag == FLAG_WRITE) {
            physicalFrame[frame].dirty = DIRTY_FRAME;
        }
        physicalFrame[frame].age = AGEMAX;

        return address;
    }
}

void initialize_mframe_manager () // Victor Chiang
{
  initialize_memory();
  // initialize memory
}

void initialize_memory_manager ()
{
  initialize_memory();
  // initialize memory and add page scan event request
}



// -----------------------------------------------------------------------------//

// --------------------- //
// Demand Paging Manager //
// --------------------- //

// implement by Surapa Phrompha

// purpose : to  handle the page fault
// this process inplement according to simos overview provided by professor

void page_fault_handler () // Surapa Phrompha
{
  unsigned *temp = (unsigned *) malloc (pageSize*sizeof(unsigned));

  int pidin = CPU.Pid;
  int instruction_page;
  int dataPage;
  int pageIn;
  int frame;

 // increment the number of page fault
  PCB[CPU.Pid]->numPF++;

  instruction_page = CPU.PC/pageSize;

  if (CPU.PTptr[instruction_page] == DISKPAGE)
  {
    printf("Page Fault has occurred for: process %d", CPU.Pid);
    printf("page %d \n",instruction_page);

      // get the free frame
      // see the process in the get free frame functions
          // obtain a free frame
          // or get a frame with the lowest age
      // if the frame is dirty, insert a write request to swapQ
      frame = get_free_frame();

      display_pagefault(frame);
      // update the frame metadata and the page tables of the involved processes
      update_frame_info(frame, CPU.Pid, instruction_page);
      update_process_pagetable(CPU.Pid, instruction_page, PENDPAGE);
      // insert a read request to swapQ to bring the new page to this frame
      insert_swapQ(pidin, instruction_page, temp, actRead, toReady);
  }
  else if (CPU.PTptr[instruction_page] == PENDPAGE)
  {
      printf("Page Fault has occurred for: process %d", CPU.Pid);
      printf("page %d \n",instruction_page);

  }
  else
  {

      dataPage = (CPU.dataOffset + CPU.IRoperand) / pageSize;
      printf("Page Fault has occurred for: process %d", CPU.Pid);
      printf("page %d \n",dataPage);

      if (CPU.PTptr[dataPage] == DISKPAGE)
      {
          frame = get_free_frame();

          display_pagefault(frame);
          update_frame_info(frame, CPU.Pid, dataPage);

          update_process_pagetable(CPU.Pid, dataPage, PENDPAGE);

          insert_swapQ(pidin, dataPage, temp, actRead, toReady);
      }
      else if (CPU.PTptr[dataPage] == PENDPAGE)
      {

      }
      else {
          printf("Page is already int the memory\n");
      }
  }
  display1FrameInfo(frame);

}

//  Page Replacement Policy (Surapa Phrompha)
// purpose : to implement an Aging Policy
// implement by scan the memory and update the age field of each frame
// Note
  // 1 : Ospages = #pages for OS, OS occupies the begining of the memory (in simoes.h)
  // 2: numFrames = sizes related to memory and memory management in (simos.h)
void memory_agescan () // Surapa Phrompha
  {
    // traverse the os page
      for(int page =OSpages; page <numFrames; page++)
        {
              // this is the memeory acces part
          if(physicalFrame[page].age!=0)
          {
           // in each scan right shit the age vector of every memory fram
            physicalFrame[page].age=physicalFrame[page].age>>1;

          }
          // when the aging vector of a memory frame become 0
          if ((physicalFrame[page].age==0) && (physicalFrame[page].free != FREE_FRAME))
          {
            // the frame is free_frame_inturn to the freelis if it is dirty.
            // fram index and fram status
              addto_freeMemoryFrame (page, physicalFrame[page].dirty);
          } // end for
        } // end function
}


//-----------------------------------------------------------------------------------------------------//

// --------------------- //
// Demand Paging Helper  //
// --------------------- //

void display_pagefault(int frame_index)
{
    if (physicalFrame[frame_index].free == USED_FRAME)
  {
        printf("There were no free frame available. \n");
    }
  else if (physicalFrame[frame_index].free == FREE_FRAME)
  {
        printf("There is freeframe\n");
    }
    display1FrameInfo (frame_index);
}


// purspose :
// get a free frame from the head of the free list
// if there is no free frame, then get one frame with the lowest age
// this func always returns a frame, either from free list or get one with lowest age
int get_free_frame ()
{
  int search_idx;
  int freeframe_idx;
  freeframe_idx = frameHead;
  // case 1 : get a free frame from the head of the frame list
  if (frameHead != NULLINDEX)
  {
      search_idx=frameHead+1;
      // travser the memory frame to seach for the free frame
      while ((physicalFrame[search_idx].free == USED_FRAME) && (search_idx <= frameTail))
      {
        search_idx++;
      }
      // end at the end of the tail


      if (search_idx > frameTail)
      {
          frameHead = NULLINDEX;
          frameTail = NULLINDEX;
      }
      else
      {
        // the free frame is at the search idx
        frameHead = search_idx;
      }
  } // end if
  else
  {
       // if there is not freeFrame
       // implement the select_agest_frame
      freeframe_idx = select_agest_frame ();
  }

//----------------------------------------------------------------------------------------------//
   // case of dirty frame
  if (physicalFrame[freeframe_idx].dirty == DIRTY_FRAME)
  {

      unsigned *buf = (unsigned *) malloc (pageSize*sizeof(unsigned));
      unsigned temp;

      // search through the page
      for (search_idx=0; search_idx<pageSize; search_idx++)
      {
          temp = (unsigned)Memory[freeframe_idx*pageSize+search_idx].mInstr;
          buf[search_idx] = temp;
      }
      // first we have to get the data from the dirry frame
      insert_swapQ(physicalFrame[freeframe_idx].pid,physicalFrame[freeframe_idx].page,buf,actWrite,Nothing);
      // then update the page table with
      update_process_pagetable (physicalFrame[freeframe_idx].pid, physicalFrame[freeframe_idx].page, DISKPAGE);
  }

  //----------------------------------------------------------------------------------------------//

  if (physicalFrame[freeframe_idx].pid != NULLINDEX)
  {
     //updates page table
     // with
      update_process_pagetable (physicalFrame[freeframe_idx].pid, physicalFrame[freeframe_idx].page, DISKPAGE);
  }

  return freeframe_idx;
} // end method


// purpose : to select the agest frame
// select a frame with the lowest age
// if there are multiple frames with the same lowest age, then choose the one
// that is not dirty
int select_agest_frame ()
{
   // low age is the highest age that we have
    int lowAge = AGEMAX;
    int frame;
    int agest_frame = NULLPAGE;


    // traverse to the total of the frame
    for(frame=OSpages; frame<numFrames; frame++)
  {
      // if it is not free frame
        if (physicalFrame[frame].free != FREE_FRAME)
      {
        if ((physicalFrame[frame].dirty == CLEAN_FRAME) && (physicalFrame[frame].age == lowAge))
        {
            agest_frame = frame;
        } // end inner if
        else if (physicalFrame[frame].age < lowAge)
        {
            lowAge = physicalFrame[frame].age;
            // update the agest frame
            agest_frame = frame;
        } // end inner else if

      } // end if
    } // end for
    return agest_frame;

}


// --------------------------------------------------------------------------------------------------------------------//

// Helper Method for loader.c


void direct_put_instruction (int frame_index, int offset, int instruction)
{
  int address = (offset & pageoffsetMask) | (frame_index << pagenumShift);
  Memory[address].mInstr = instruction;
}

void direct_put_data (int frame_index, int offset, mdType data)
{
  int address = (offset & pageoffsetMask) | (frame_index << pagenumShift);
  Memory[address].mData = data;
}
