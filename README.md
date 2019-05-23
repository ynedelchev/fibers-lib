Native C Fibres Library
=======================

Compile
-------
Use the `Makefile` provided to build the library
```
   make
```

As a result, you would get an object file named `fibers.o`, that you could use to link into your project.


Clean
-----
To clean all compiler output files and prepare for a clean new build, you may execute:
```
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
````
#import "fibers.h"
````

When creating a new fiber, you need to specify the procedure/routine that would be executed when the fiber starts.
That is done by calling `fiber_create(....)` like this:
````
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
```` 

Please note that each fiber routine should end up calling `fiber_exit();` whent it exits.
The information for the newly created fiber is stored in a pre-allocated `struct fiber` structure that is passed as the first parameter of `fiber_create(....)`.

At the moment when a the fiber routine is logically blocked waiting for some external condition, it should call `fiber_yell()`, thus allowing the fiber library 
to switch context and give execution to some other fiber routine. 

For example, if you are implementing a Producer-Consumer algorithm, the producer would start producing stock until the space it has is completely full.
At that moment, it should stop producing and wait for the consumer to consume some of the stocks. 
At that point the routine of the producer should call `fiber_yell()` to give control to the consumer, which runs in a nother fiber. 

Once, you have created all of the fibers you would need and have defined the routine for each ofthem, you need to start execution by giving control to the first 
one. You can do that by calling: 
````
fibers_start_first();`
````


License
-------
[MIT](https://opensource.org/licenses/MIT)
