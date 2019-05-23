#include "fibers.h"
#include <ucontext.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


/* This is a new internally used element for building a list of created 
 * fibers. This would be a double linked list of fibers, so that we can 
 * easily add and remove elements from/to the list. 
 * each fiber (fiber_t) that is created would also have a link to its 
 * corresponding fiber_element in the list, so it knows its place in the 
 * list and we do not need to parse the list to find it. 
 *
 * At any given momemnt there would be no more than one active fiber.
 * the active fiber will defined by the current_fiber pointer. 
 * 
 * There is always at least one lement in the list - the main_fiber 
 * element. That is always linked to the fiber broker and would always 
 * have an id of 0.
 */
struct fiber_element {
   fiber_t* fiber;     
   /*
    * This element shows if the fiber member varaible has been allocated
	* by the fiber_create function and therefore should be freed once 
	* the fiber_exit() function is called.
	* Indeed the actual freeing of the resources will happen in the
	* fiber_broker_function.
	*/
   int is_fiber_allocated;     
   /*
    * This pending_removal flag is set to true (1) when the fiber_exit()
	* function is called. It instructs the fiber_broker_function to 
	* free all resourceis associated with this fiber element. 
	* if the flag 'is_fiber_allocated' is set, then the fiber element
	* is freed as well.
	*/
   int is_pending_removal;
   struct fiber_element* next; // Next element in the fiber list.
   struct fiber_element* prev; // Previous element in the fiber list.
};
struct fiber_element  main_fiber = { NULL, 0, 0,   NULL, NULL};
struct fiber_element* first = &main_fiber;   // The first fiber in the list.
struct fiber_element* last  = &main_fiber;   // The last fiber in the list.
struct fiber_element* current_fiber = NULL;  // The current active fiber.


const int stack_size = 16384;  // The default size of a stack in a new fiber


static int fiber_remove(struct fiber_element* el);
/**
 * This function is used to generate new unique (or at least very likely to
 * be unique) identifier for each new fiber that is being created. 
 * It would never return 0 and it would never return a negative number. 
 * RETURN VALUE
 *     A very likely to be unique value that can be used as identifier for 
 *     any new fiber.
 */
static int get_new_id() {
   long int id = 0;
   int count = 0;
   while (id == 0 && count < 100) {
       id = abs(random());
       count++;
   }
   return id == 0 ? 1 : id;
}


static void fiber_broker_function() {
   printf("Inside fiber broker function.\n");
   // Synchronization start
   // Find inactive fiber.
   long fromid = 0;
   long toid = 0;
   struct fiber_element* from = current_fiber;
   struct fiber_element* to = from->next;
   fromid = from == NULL ? 0 : from->fiber->id;
   toid   = to   == NULL ? 0 : to->fiber->id; 
   if (to == NULL) {
      to = main_fiber.next;
   }
   if (to == from) {
      to = NULL;
   }
   current_fiber = to;
   if (from->is_pending_removal) {
      fiber_remove(from);
   }
   // Synchronization end
   if (to != NULL) {
      toid   = to   == NULL ? 0 : to->fiber->id; 
	  printf("Switching context from fiber %li to fiber %li\n", 
	      (long int)fromid, (long int)toid);
      int result = setcontext(to->fiber->uc_link);
	  if (result == -1) {
	     perror("setcontext to fiber failed");
		 return;
	  }
   }
   // Just end everything.
}

static int initialize_broker() {
    if (main_fiber.fiber != NULL) {
        return 0;
    }
    // Synchronization start
    main_fiber.fiber = (fiber_t*)calloc(1, sizeof(fiber_t));
    if (main_fiber.fiber == NULL) {
       return 1;
    }
    main_fiber.fiber->uc_link = (ucontext_t*)calloc(1, sizeof(ucontext_t));
    if (main_fiber.fiber->uc_link == NULL) {
       free(main_fiber.fiber);
       return 2;
    }
    int result = getcontext(main_fiber.fiber->uc_link);
    if (result == -1) {
       free(main_fiber.fiber->uc_link); main_fiber.fiber->uc_link = NULL;
       free(main_fiber.fiber); main_fiber.fiber = NULL;
       return 3;
    }
    main_fiber.fiber->uc_link->uc_stack.ss_sp = (char*)calloc(stack_size, sizeof(char));
    if (main_fiber.fiber->uc_link->uc_stack.ss_sp == NULL) {
       free(main_fiber.fiber->uc_link); main_fiber.fiber->uc_link = NULL;
       free(main_fiber.fiber); main_fiber.fiber = NULL;
       return 4;
    }
    main_fiber.fiber->uc_link->uc_stack.ss_size = stack_size;
	main_fiber.fiber->fiber_element = &main_fiber;
    makecontext(main_fiber.fiber->uc_link, fiber_broker_function, 0);
    // Synchronization end
    return 0;
}



/* 
 * This function will initialize a new fiber element and will add it to the 
 * list of fibers. The newly added fiber will be associated with the function
 * start_routine, but this method will not automatically start it a call 
 * that method. Any information about this fiber will be stored in the fiber
 * parameter. 
 *
 * 
 * RETURN VALUE: On success, fiber_add(..) returns 0; on error, it returns
 * an error number, and the content of *fiber are undefined.
 */
int fiber_create(fiber_t* fiber, const fiber_attr_t* attr, void (*start_routine)(void)) {

    if (initialize_broker()) {return -1;}
    int fiber_is_allocated = 0;

    if (fiber == NULL) {
       fiber = (fiber_t*)calloc(1, sizeof(fiber_t));
       if (fiber == NULL) {
           return 1;
       }
       fiber_is_allocated = 1; // true
    }

    struct fiber_element* el = (struct fiber_element*)calloc(1, sizeof(struct fiber_element));
    el->fiber = fiber;
	el->is_fiber_allocated = fiber_is_allocated;  //
	el->is_pending_removal = 0;                   // false
    el->next  = NULL;
    el->prev  = NULL;

    fiber->id = fiber->id != 0 ? fiber->id : get_new_id();
    fiber->fiber_element = el;

    int ssize = stack_size;
    if (attr != NULL && attr->stack_size != 0) {
       ssize = attr->stack_size;
    }
    void* stack = (void*)calloc(ssize, sizeof(char));
    if (stack == NULL) {
        free(fiber_is_allocated ? fiber : NULL);
        free(el);
        return 2;
    }
    fiber->uc_link = (ucontext_t*)calloc(1, sizeof(ucontext_t));
    if (fiber->uc_link == NULL) {
        free(fiber_is_allocated ? fiber : NULL);
        free(el);
        free(stack);
        return 3;
    }

    int result = getcontext(fiber->uc_link);
    if (result == -1) {
       free(fiber_is_allocated ? fiber : NULL);
       free(stack);          stack = NULL;
       free(fiber->uc_link); fiber->uc_link = NULL;
       free(el);             el = NULL;
       return 4;
    }

    fiber->uc_link->uc_stack.ss_sp   = stack;            // Stack allocated for new ctx
    fiber->uc_link->uc_stack.ss_size = stack_size;       // Stack size of new context
    fiber->uc_link->uc_link = main_fiber.fiber->uc_link; // Previous conetxt
       
    makecontext(fiber->uc_link, start_routine, 0);
    
    // Sync start
    el->prev = last;
    last->next = el;
    last = el;
    // Sunc end

    return 0;
}

int fibers_start(fiber_t* fiber) {
    if (initialize_broker()) {return -1;}
    if (fiber == NULL) {
       fiber = main_fiber.fiber;
    }
    if (fiber == NULL) {
       return 1;
    }
    if (fiber->uc_link == NULL) {
       return 2;
    }
    if (fiber->id == 0) {
	   struct fiber_element* el = fiber->fiber_element;
	   if (el == NULL) {
	      return 3;
	   }
	   if (el->next == NULL) {
	      return 5;
	   }
	   if (el->next->fiber == NULL) {
	      return 6;
	   }
	   fiber = el->next->fiber;
    }
    current_fiber = fiber->fiber_element;
    int result = setcontext(fiber->uc_link);
    if (result == -1) {
       return 7;
    }
    return 0;
}

int fibers_start_first() {
   return fibers_start(NULL);
}



/* This will pass the controll to some other fiber.
 */
static int fiber_remove(struct fiber_element* el) {
    if (initialize_broker()) {return -1;}
	if (el == NULL) {
	    return 0;
	}
    if (el == current_fiber) {
		fprintf(stderr, "Trying to call fiber_remove(...) and remove current "
		"fiber with id %li from the list of available fibers. Instead please "
		"call fiber_exit() and just exit the function of this fiber. Just "
		"ignoring the call to fiber_remove(...).\n", 
		(long int)( el->fiber != NULL ? el->fiber->id : -1));
		return 1;
    }

    // Synchrnonize start
    el->prev->next = el->next;
    // Synchronize end
	fiber_t* fiber = el->fiber;

    if (fiber->uc_link != NULL) { 
       memset(fiber->uc_link->uc_stack.ss_sp, '\0', 
              fiber->uc_link->uc_stack.ss_size);
       free(fiber->uc_link->uc_stack.ss_sp);
       memset(fiber->uc_link, '\0', sizeof(ucontext_t));
       free(fiber->uc_link);
	   if (el->is_fiber_allocated) {
	      free(el->fiber);
	   }
    }
	el->fiber = NULL;
    free(el);
    return 0;
}

int fiber_exit() {
    if (initialize_broker()) {return -1;}
    // Synchrnonize start
    if (current_fiber && current_fiber->prev) {
	    current_fiber->is_pending_removal = 1;  
    }	
    // Synchronize end
    return 0;
}

int fiber_yell() {
    if (initialize_broker()) {return -1;}
    // Synchronize start
    swapcontext(current_fiber->fiber->uc_link, main_fiber.fiber->uc_link);  
    // Synchronize end
    return 0;
}

