#include <pthread.h>
#include <stdio.h>
#include "simos.h"

//===============================================================
// The interface to interact with clients
// current implementation is to get one client input from stdin
// --------------------------
// Should change to create server socket to accept client connections
// and then poll client ports to get their inputs.
// -- Best is to use the select function to get client inputs
// Also, during execution, the terminal output (term.c) should be
// sent back to the client and client prints on its terminal.
//===============================================================

void submission ()
{ char fname[100];

  printf ("Submission file: ");
  scanf ("%s", &fname);
  if (Debug) printf ("File name: %s has been submitted\n", fname);
  submit_process (fname);
}

void *process_submissions ()
{ char action;
  char fname[100];

  while (systemActive) submission ();

}
