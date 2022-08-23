#define _XOPEN_SOURCE 500
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include "simos.h"

//======================================================================
// This module handles swap space management.
// It has the simulated disk and swamp manager.
// First part is for the simulated disk to read/write pages.
//======================================================================


#define swapFname "swap.disk"
#define itemPerLine 8
int diskfd;
int swapspaceSize;
int PswapSize;
int pagedataSize;

sem_t swap_mutex;
sem_t disk_mutex;
sem_t swap_semaphore;

//===================================================
// This is the simulated disk, including disk read, write, dump.
// The unit is a page
//===================================================
// each process has a fix-sized swap space, its page count starts from 0
// first 2 processes: OS=0, idle=1, have no swap space
// OS frequently (like Linux) runs on physical memory address (fixed locations)
// virtual memory is too expensive and unnecessary for OS => no swap needed


// move to proper file location before read/write/dump
int move_filepointer (int pid, int page)
{ int currentoffset, newlocation, ret;

  if (pid <= idlePid || pid > maxProcess)
  { printf ( "Error: Incorrect pid for disk dump: %d\n", pid);
    exit (-1); }
  if (page < 0 || page > maxPpages)
  { printf ( "Error: Incorrect page number: pid=%d, page=%d\n",
                   pid, page);
    exit (-1);
  }
  currentoffset = lseek (diskfd, 0, SEEK_CUR);
  newlocation = (pid-2) * PswapSize + page*pagedataSize;
  ret = lseek (diskfd, newlocation, SEEK_SET);
  if (ret < 0)
  { printf ( "Error lseek in move: ");
    printf ( "pid/page=%d,%d, loc=%d,%d, size=%d\n",
             pid, page, currentoffset, newlocation, pagedataSize);
    exit (-1);
  }
  return (currentoffset);
    // currentoffset cannot be larger than 2G
}

// originally prepared for dump, now not in use
void moveback_filepointer (int location)
{ int ret;

  ret = lseek (diskfd, location, SEEK_SET);
  if (ret < 0)
  { printf ( "Error lseek in moveback: ");
    printf ( "location=%d\n", location);
    exit (-1);
  }
}


// read/write are called within swap thread, dump is called externally
// if called concurrently, they may change file location => error IO
// Solution: use mutex semaphore to protect them
// each function recomputes address, so there will be no problem

int read_swap_page (int pid, int page, unsigned *buf)
{ int ret;

  move_filepointer (pid, page);
  ret = read (diskfd, (char *)buf, pagedataSize);
  if (ret != pagedataSize)
  { printf ( "Error: Disk read returned incorrect size: %d\n", ret);
    exit(-1);
  }
  usleep (diskRWtime);  // simulate the delay for disk RW
}

int write_swap_page (int pid, int page, unsigned *buf)
{ int ret;

  move_filepointer (pid, page);
  ret = write (diskfd, (char *)buf, pagedataSize);
  if (ret != pagedataSize)
  { printf ( "Error: Disk write returned incorrect size: %d\n", ret);
    exit(-1);
  }
  usleep (diskRWtime);  // simulate the delay for disk RW
}



int dump_process_swap_page (int pid, int page)
{ int oldloc, ret, retsize, k;
  int buf[pageSize];


  int tInstr, tOpcode, tOperand;
  mType *temp = (mType *) malloc (sizeof(mType));

  oldloc = move_filepointer (pid, page);
  ret = read (diskfd, (char *)buf, pagedataSize);
  if (ret != pagedataSize)
  { fprintf (infF, "Error: Disk dump read incorrect size: %d\n", ret);
    exit (-1);
  }

  printf("----------------------------------------\n" );
  printf ("Content of process %d swap page %d:\n", pid, page);
  for (k=0; k<pageSize; k++) {

	  temp->mInstr = buf[k];
	  printf ("0x%016x|%.2f \n", buf[k],temp->mData);
  }
  printf ("\n");

}

void dump_process_swap (int pid)
{ int j;

  printf ("****** Dump swap pages for process %d\n", pid);
  for (j=0; j<maxPpages; j++) dump_process_swap_page (pid, j);
}

// open the file with the swap space size, initialize content to 0
void initialize_swap_space ()
{ int ret, i, j, k;
  int buf[pageSize];

  swapspaceSize = maxProcess*maxPpages*pageSize*dataSize;
  PswapSize = maxPpages*pageSize*dataSize;
  pagedataSize = pageSize*dataSize;

  diskfd = open (swapFname, O_RDWR | O_CREAT, 0600);
  if (diskfd < 0) { perror ("Error open: "); exit (-1); }
  ret = lseek (diskfd, swapspaceSize, SEEK_SET);
  if (ret < 0) { perror ("Error lseek in open: "); exit (-1); }
  for (i=2; i<maxProcess; i++) {
    for (j=0; j<maxPpages; j++)
    { for (k=0; k<pageSize; k++) buf[k]=0;
      write_swap_page (i, j, buf);
    }
	dump_process_swap(i);
  }
    // last parameter is the origin, offset from the origin, which can be:
    // SEEK_SET: 0, SEEK_CUR: from current position, SEEK_END: from eof
}


//===================================================
// Here is the swap space manager. Its main job is SwapQ management.
// Swap manager (disk device) runs in paralel with CPU-memory
// A swap request (read/write a page) is inserted to the swapQ (queue)
//    by other processes (loader, memory)
// Swap manager takes jobs from SwapQ and process them
// After a job is completed, it is pushed to endIO queue
//===================================================

typedef struct SwapQnodeStruct
{ int pid, page, act, finishact;
  unsigned *buf;
  struct SwapQnodeStruct *next;
} SwapQnode;
// pidin, pagein, inbuf: for the page with PF, needs to be brought in
// pidout, pageout, outbuf: for the page to be swapped out
// if there is no page to be swapped out (not dirty), then pidout = nullPid
// inbuf and outbuf are the actual memory page content

SwapQnode *swapQhead = NULL;
SwapQnode *swapQtail = NULL;

void print_one_swapnode (SwapQnode *node)
{ printf ("pid,page=(%d,%d), act,fact=(%d, %d), buf=%x \n",
           node->pid, node->page, node->act, node->finishact, node->buf);
}

void dump_swapQ ()
{ SwapQnode *node;

  printf ("******************** Swap Queue Dump\n");
  node = swapQhead;
  while (node != NULL) {
    printf ("pid,page=(%d,%d), act,fact=(%d, %d), buf=%x \n",
               node->pid, node->page, node->act, node->finishact, node->buf);
	  node = node->next;
  }

  printf ("\n");
}

void process_one_swap()
{
  SwapQnode *node;
  int i, frame, ret;
  mType *buf = (mType *) malloc (pageSize*sizeof(mType));

  sem_wait(&swap_semaphore);
  sem_wait(&swap_mutex);

  if (swapQhead == NULL)
  { printf ("\aError: No process in swap queue!!!\n");
    sem_post(&swap_mutex);
  }
  else
  {
	  node = swapQhead;
	  printf ("\nPage Fault Handler: pid,page=(%d,%d), act,ready=(%d, %d), buf=%x,\n",
           node->pid, node->page, node->act, node->finishact, node->buf);
	  if (node->act == actWrite)
    {
      //write to swapDisk from memory
		  write_swap_page(node->pid, node->page, node->buf);
		  for (i=0;i<pageSize;i++)
      {
			  buf[i].mInstr = (int)node->buf[i];
			  if (PCB[node->pid]->dataOffset <= (node->page*pageSize+i))
        {
				  printf("Data: 0x%016x %f \n", buf[i], buf[i].mData);

			  }
			  else {
				  printf("Instruction: 0x%016x\n", buf[i]);

			  } // end else
		  } // end for
	  } // end if
	  else if (node->act == actRead)
    {
      //read from disk, then send to load_data or load_instruction
		  node->buf = (unsigned *) malloc (pageSize*sizeof(unsigned));
		  read_swap_page(node->pid, node->page, node->buf);
		  frame = find_allocated_memory(node->pid, node->page);
		  if (frame < 0)
      {
        //if no frame returned, just remove node and exit
			  update_process_pagetable (node->pid, node->page, -1);
			  if (swapDebug)
        {
          printf ("Remove swap queue %d %d\n", node->pid, node->page);
        }
			  swapQhead = node->next;
			  if (swapQhead == NULL)
        {
          swapQtail = NULL;
        }
			  free (node->buf); free (node);
			  sem_post(&swap_semaphore); sem_post(&swap_mutex);
			  if (swapQhead == NULL) sem_wait(&swap_semaphore);
			  return;
      }
		   update_frame_info (frame, node->pid, node->page);
		   update_process_pagetable (node->pid, node->page, frame);
		  for (i=0;i<pageSize;i++)
      {
			  buf[i].mInstr = (int)node->buf[i];
			  if (PCB[node->pid]->dataOffset <= (node->page*pageSize+i))
        {
				  printf("Data: 0x%016x %f \n", buf[i], buf[i].mData);
				  ret=load_data (&buf[i], frame, i);
				  if (ret == mError)
          {
            PCB[node->pid]->exeStatus = eError;
          } // end if

			  } // end if
			  else {
				  printf("Instruction: 0x%016x\n", buf[i]);
				  ret=load_instruction (&buf[i], frame, i);
				  if (ret == mError)
          {
            PCB[node->pid]->exeStatus = eError;
          }

			  }
		  }


		 if (node->finishact == toReady || node->finishact == Both)
      {

        // ADDCODE HERE //
			  insert_endIO_list(node->pid);
			  endIO_moveto_ready ();
      }
	  }


	  if (swapDebug)
    {
      printf ("Remove swap queue %d %d\n", node->pid, node->page);
    }
	  swapQhead = node->next;
	  if (swapQhead == NULL) swapQtail = NULL;
      free (node->buf); free (node);
	  sem_post(&swap_semaphore); sem_post(&swap_mutex);
	  if (swapQhead == NULL) sem_wait(&swap_semaphore);
  }
}


void insert_swapQ (pid, page, buf, act, finishact)
int pid, page, act, finishact;
unsigned *buf;
{ SwapQnode *node; unsigned *temp = (unsigned *) malloc (pageSize*sizeof(unsigned));
  int i; unsigned temp2;

  sem_wait(&swap_mutex);

  if (swapDebug) printf ("Insert swapQ %d %d %d %d\n", pid, page, act, finishact);

  node = (SwapQnode *) malloc (sizeof (SwapQnode));
  node->pid = pid;
  node->page = page;
  node->act = act;
  node->finishact = finishact;

  node->next = NULL;
  if (act == actWrite)
  {
	  node->buf = (unsigned *) malloc (pageSize*sizeof(unsigned));
	  for (i=0;i<pageSize;i++){
		  printf("0x%016x ",buf[i]);
		  temp2 = buf[i];

		  node->buf[i] = temp2;
	  }
  }
  else if (act == actRead)
  {
	  node->buf = buf;
  }


  if (swapQhead==NULL)
  {
	  swapQhead = node; swapQtail=node;
  }
  else {
	  swapQtail->next = node; swapQtail = node;
  }
  if (swapDebug) dump_swapQ ();
  sem_post(&swap_semaphore);
  sem_post(&swap_mutex);
  if (swapQhead!=node) sem_wait(&swap_semaphore);
}

void *process_swapQ ()
{
  while (systemActive) process_one_swap ();
  if (swapDebug) printf ("swapQ loop has ended\n");

}

pthread_t swapThread;

void start_swap_manager ()
{ int ret;

   // initialize semaphores
  sem_init(&swap_semaphore,0,1);
  sem_init(&swap_mutex,0,1);
  sem_init(&disk_mutex,0,1);
  // initialize_swap_space ();
  initialize_swap_space ();
  sem_wait(&swap_semaphore);
  printf ("Swap space manager is activated.\n");
  // create swap thread
  ret = pthread_create (&swapThread, NULL, process_swapQ, NULL);
  if (ret < 0) printf ("Error in Swap Thread Creatiom\n");
  else printf ("Swap thread has been created successsfully\n");
}

void end_swap_manager ()
{
  // terminate the swap thread
  sem_post(&swap_semaphore); sem_post(&swap_mutex); //**DG
  close (diskfd);
  pthread_join (swapThread, NULL);
  printf ("Swap Manager thread has terminated %d.\n");

}
