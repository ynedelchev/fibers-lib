#include "fibres.h"
#include <stdio.h>
#include <strings.h>


#define BUFF_SIZE 5  // total number of slots
#define NP        1  // total number of producers
#define NC        1  // total number of consumers
#define NITERS    4  // number of items produced/consumed


typedef struct {
   int buf[BUFF_SIZE];    // shared var
   int in;                // buf[in  % BUFF_SIZE] is the first empty slot
   int out;               // buf[out % BUFF_SIZE] is the first full slot
   int full;              // keep track of the number of full slots
   int empty;             // keep track of the number of empty slots
} sbuf_t;

sbuf_t shared;

void Producer(void) 
{
   int i, item, index;
   int argi = 0;
   void* arg = &argi; 
   printf("Starting Producer %li\n", (long)arg);

   index = (int)arg;

   for (i =0; i < NITERS; i++) {
      // Produce item.
	  item = i;

	  // Prepare to write item to buf
	  // if there are no empty slots, wait
	  while (shared.empty == 0) {
	     fiber_yell();
	  }
	  // If another thread uses the buffer, wait
	  shared.buf[shared.in] = item;
	  shared.in = (shared.in + 1) % BUFF_SIZE;
	  printf("[P%d] + Producing %d ... \n", index, item);
	  fflush(stdout);
	  // Release the buffer
	  // Increment the number of full slots
	  shared.full++;
	  shared.empty--;

	  // Interleave producer and consumer execution
	  if (i % 2 == 1) {
	      fiber_yell();
	  }
   }
   printf("Producer %li ended.\n", (long)arg);
   fiber_exit();
}



void Consumer(void) {
   // fill in the code here.
   int i = 0;
   int argi = 0;
   void* arg = &argi; 
   int value = 0;
   int index = (int)arg;
   for (i =0; i < NITERS; i++) {
	  while (shared.full == 0) {
	     fiber_yell();
	  }
	  value = shared.buf[shared.out];
	  shared.buf[shared.out] = 0;
	  shared.out = ( shared.out + 1 ) % BUFF_SIZE;
	  printf("[C%d]   Consuming %d .. with value %d\n", index, i, value);
	  fflush(stdout);
	  shared.full--;
	  shared.empty++;
	  if (i % 2 == 1) {
	     fiber_yell();
	  }
   }
   printf("Consumer %li ended.\n", (long)arg);
   fiber_exit();
}



int main(int argc, char* argv[]) {

	int index;

	bzero(&shared, sizeof(shared));
	shared.in  = 0;           // shared.buf[shared.in  % BUFF_SIZE] is the first empty slot
	shared.out = 0;           // shared.buf[shared.out % BUFF_SIZE] is the first full slot
	shared.full = 0;          // No full slots
	shared.empty = BUFF_SIZE; // All slots are empty 

	fiber_t Producers[NP];
	fiber_t Consumers[NC];
    
	bzero(Producers, sizeof(Producers));
	bzero(Consumers, sizeof(Consumers));

	for (index = 0; index < NP; index++) {
	    Producers[index].id = 1000 + index;
	    // Create new producer 
		fiber_create(&Producers[index], NULL, Producer);
	}

	for (index = 0; index < NC; index++) {
		Consumers[index].id = 2000 + index;
	    fiber_create(&Consumers[index], NULL, Consumer);
	}

    fibers_start_first();

    return 0;
}
