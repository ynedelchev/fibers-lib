Native C Fibres Library
=======================
<!-- Please use markdown-toc -i README.md to update the table of contents -->
<!-- -->
**Table of Contents**  *generated with [Markdown-TOC](https://www.npmjs.com/package/markdown-toc#install)*

<!-- toc -->

- [Compile](#compile)
- [Clean](#clean)
- [What are fibers](#what-are-fibers)
- [How to use this library](#how-to-use-this-library)
- [Example (Producer - Consumer Problem)](#example-producer---consumer-problem)
- [License](#license)

<!-- tocstop -->

Compile
-------
Use the `Makefile` provided to build the library
```bash
   make
```

As a result, you would get an object file named `fibers.o`, that you could use to link into your project.


Clean
-----
To clean all compiler output files and prepare for a clean new build, you may execute:
```bash
  make clean
```

What are fibers
---------------
Fibres allow single threaded applications to switch context and stop executing one process flow and switching to another. 
Unlike threads the two process flows cannot run in parallel. It is only when the first one is blocked and thus stops execution, then the other one can continue. 
Fibers are very suitable for implementation of Producer-Consumer problems and are lighter than threads in terms of resources.

How to use this library
-----------------------
First, you need to  include the [fibers.h](https://github.com/ynedelchev/fibers-lib/blob/master/fibers.h) header file into your C program.
```c
#import "fibers.h"
```

When creating a new fiber, you need to specify the procedure/routine that would be executed when the fiber starts.
That is done by calling `fiber_create(....)` like this:
```c
void Producer(void) 
{
   ...
   fiber_exit();
}

...

struct fiber_attr attributes;
attributes.stack_size = 0; // default
attributes.fiber_id = 0;   // create new random id

struct fiber procuder;

fiber_create(&producer, &attributes, Producer);
``` 

Please note that each fiber routine should end up calling `fiber_exit();` whent it exits.
The information for the newly created fiber is stored in a pre-allocated `struct fiber` structure that is passed as the first parameter of `fiber_create(....)`.

At the moment when a the fiber routine is logically blocked waiting for some external condition, it should call `fiber_yell()`, thus allowing the fiber library 
to switch context and give execution to some other fiber routine. 

For example, if you are implementing a Producer-Consumer algorithm, the producer would start producing stock until the space it has is completely full.
At that moment, it should stop producing and wait for the consumer to consume some of the stocks. 
At that point the routine of the producer should call `fiber_yell()` to give control to the consumer, which runs in a nother fiber. 

Once, you have created all of the fibers you would need and have defined the routine for each ofthem, you need to start execution by giving control to the first 
one. You can do that by calling: 
```c
fibers_start_first();`
```

Example (Producer - Consumer Problem)
------------------------------------

The Producer Consumer problem consists of the following:

You have one or more producers that produce items and put them in a storage of limited space. 
At the same time, you also have one or more consumers, that get items from the storage freeing space and process (consume) them.


If the producers are faster than the consumers, then at some point they would fill up the whole storage space, so there is no space for newly produced items.
At that point the producers should stop and wait untill some of the items are consumed (thus removed from the storage) and some space is freed for new items.

On the contrary, if the consumers are faster than the producers, at some point they would consume all of the items already produced and the storage will be empty.
At that point consumers should stop and wait untill some new items are produced.

Now let's try to create an implementation of the Producer - Consumer problem, where each producer and each consumer would be a fiber.
Each time when a producer or consumer needs to stop and wait, they would call the `fiber_yell()` function allowing other fiber to catch in.

Lets do some initialization
```c
#include "fibers.h"
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
```

Here `shared` is our shared storage between producers and consumers.

```c
typedef struct {
   int buf[BUFF_SIZE];    
   int in;-------------------------+                
   int out;------\                 |
   int full;     |                 |
   int empty;    |                 |
} sbuf_t;        |                 |
                 |                 |
                 V                 V
       +---+---+---+-- ... --+---+---+---+
 buf   |   |   | X |         | X | X |   |
       +---+---+---+-- ... --+---+---+---+
 index   0   1   2                    BUFF_SIZE-1
                
                 |                 |
                 \_______   ______/
                          V
                         full 
                     (number of items)
```

Here is the producer function/routine:
```c
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
```

Please note that when the producer exits it *must* call `fiber_exit()`. That is a prerequisite for the fiber lib to work properly.

Then here is the Consumer:
```c
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
```

Now lets create a number of producers and consumers. Each one of them is a new fiber that references the corresponding routine `Producer(....)` or `Consumer(....)`.
```c
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
``` 

Let's say, that you have your program in file named `test.c`.
Compile that with 
```bash
gcc -c test.c -o test.o
gcc test.o fibers.o -o test 
```

where `fibers.o` is the output after compiling the fibers lib library with `gcc -c fibers.c -o fibers.o` command.
The result would be an executable file `test`. 
Run it like 
```bash
 ./test
```
and see what happens.

License
-------
[MIT](https://opensource.org/licenses/MIT)
