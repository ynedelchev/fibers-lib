#ifndef FIBER_LIBRARY_H
#define FIBER_LIBRARY_H


#include <ucontext.h>


struct fiber {
   ucontext_t* uc_link;
   int id;
   void* fiber_element;
};


/*
 * Attributes definig the different parameters that will be ued when 
 * creating new fibers.
 */
struct fiber_attr {

   /*
    * The stack size to be used for the new fiber.
    * Set this to zero (0) so that the default size be used. 
    */
   int stack_size;      

   /* 
    * The id that will be ued for the newly created fiber. Please set that 
    * to 0 to force creating new randomly generated id.
    */
   int fiber_id;     
};


typedef struct fiber fiber_t;
typedef struct fiber_attr fiber_attr_t;





/* 
 * This function will initialize a new fiber element and will add it to the 
 * list of fibers. The newly added fiber will be associated with the 
 * function start_routine, but this method will not automatically start it 
 * a call that method. Any information about this fiber will be stored in 
 * the fiber parameter. 
 *
 * 
 * RETURN VALUE: On success, fiber_add(..) returns 0; on error, it returns
 * an error number, and the content of *fiber are undefined.
 */
int fiber_create(fiber_t* fiber, const fiber_attr_t* attr, void (*start_routine)(void));

int fibers_start(fiber_t* fiber);

int fibers_start_first();

int fiber_exit();

int fiber_yell();


#endif
