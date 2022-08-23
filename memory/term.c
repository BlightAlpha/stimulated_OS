#define _XOPEN_SOURCE 500
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include "simos.h"

//=========================================================================
// Terminal manager is responsible for printing an output string to terminal,
//    which is simulated by a file "terminal.out"
// Terminal is a separate device from CPU/Mem, so it is a separate thread.
//    Some accesses interfere with the main thread and protection is needed
//    Use semaphores to make it thread safe under concurrent accesses
// When there is an output, the process has to be in eWait state and
//    We insert both pid and the output string to the terminal queue.
// Terminal manager manages the queue and process printing jobs in the queue
// After printing is done, we put the process back to the ready state,
//    which has to be done by the process manager.
//    Terminal only puts the process in endIO queue and set the interrupt.
//=========================================================================

// terminal output file name and file descriptor
// write terminal output to a file, to avoid messy printout

#define termFN "terminal.out"
FILE *fterm;

void terminal_output (pid, outstr)
int pid;
char *outstr;
{
  fprintf (fterm, "%s\n", outstr);
  fflush (fterm);
  usleep (termPrintTime);
}

//=========================================================================
// terminal queue
// implemented as a list with head and tail pointers
//=========================================================================


typedef struct TermQnodeStruct
{ int pid, type;
  char *str;
  struct TermQnodeStruct *next;
} TermQnode;

TermQnode *termQhead = NULL;
TermQnode *termQtail = NULL;


sem_t term_semaphor;


sem_t term_mutex;


// dump terminal queue is not called inside the terminal thread,
// only called by admin.c
void dump_termIO_queue (FILE *outf)
{
  TermQnode *node;

  fprintf (outf, "******************** Term Queue Dump\n");
  node = termQhead;
  while (node != NULL)
  { fprintf (outf, "%d, %s\n", node->pid, node->str);
    node = node->next;
  }
  fprintf (outf, "\n");
}

// insert terminal queue is not inside the terminal thread, but called by
// the main thread when terminal output is needed (only in cpu.c, process.c)
void insert_termIO (pid, outstr, type)
int pid, type;
char *outstr;
{ TermQnode *node;

  sem_wait(&term_mutex);

  if (termDebug) printf ("Insert term queue %d %s\n", pid, outstr);
  node = (TermQnode *) malloc (sizeof (TermQnode));
  node->pid = pid;
  node->str = outstr;
  node->type = type;
  node->next = NULL;
  if (termQtail == NULL) // termQhead would be NULL also
  { termQtail = node; termQhead = node; }
  else // insert to tail
  { termQtail->next = node; termQtail = node; }
  if (termDebug) dump_termIO_queue (bugF);
  sem_post(&term_semaphor);
  sem_post(&term_mutex);
}

// remove the termIO job from queue and call terminal_output for printing
// after printing, put the job to endWait list and set endWait interrupt
void handle_one_termio ()
{ TermQnode *node;

  sem_wait(&term_semaphor);
  sem_wait(&term_mutex);

  if (termDebug) dump_termIO_queue (bugF);
  if (termQhead == NULL)
  { printf ("No process in terminal queue!!!\n"); sem_post(&term_mutex); sem_post(&term_semaphor);}
  else
  { node = termQhead;
    terminal_output (node->pid, node->str);
    if (node->type != exitProgIO)
    {
      insert_endIO_list (node->pid);
      set_interrupt (endIOinterrupt);
    }   // if it is the exitProgIO type, then job done, just clean termio queue

    if (termDebug) printf ("Remove term queue %d %s\n", node->pid, node->str);
    termQhead = node->next;
    if (termQhead == NULL) termQtail = NULL;
    free (node->str); free (node);
    if (termDebug) dump_termIO_queue (bugF);
	sem_post(&term_semaphor); sem_post(&term_mutex);
	if (termQhead == NULL) sem_wait(&term_semaphor);
  }
}

//=====================================================
// loop on handle_one_termIO to process the termIO requests
// This has to be a separate thread to loop for request handling
//=====================================================



void *termIO ()
{
  while (systemActive) handle_one_termio ();
  if (termDebug) printf ("TermIO loop has ended\n");
}

pthread_t termThread;

void start_terminal ()
{ int ret;
  sem_init(&term_semaphor,0,1);
  sem_init(&term_mutex,0,1);
  sem_wait(&term_semaphor);

  fterm = fopen (termFN, "w");
  ret = pthread_create (&termThread, NULL, termIO, NULL);
  if (ret < 0) printf ("Error ! TermIO Thread Creation \n");
  else printf ("TermIO thread is created\n");
}

void end_terminal ()
{ int ret;
  sem_post(&term_semaphor); sem_post(&term_mutex); //**DG

  fclose (fterm);
  ret = pthread_join (termThread, NULL);
  printf ("TermIO thread has terminated %d\n", ret);
}
