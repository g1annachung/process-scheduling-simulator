/**********************************************************************
 * Copyright (c) 2020
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

/* THIS FILE IS ALL YOURS; DO WHATEVER YOU WANT TO DO IN THIS FILE */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "list_head.h"

/**
 * The process which is currently running
 */
#include "process.h"
extern struct process *current;

/**
 * List head to hold the processes ready to run
 */
extern struct list_head readyqueue;


/**
 * Resources in the system.
 */
#include "resource.h"
extern struct resource resources[NR_RESOURCES];


/**
 * Monotonically increasing ticks
 */
extern unsigned int ticks;


/**
 * Quiet mode. True if the program was started with -q option
 */
extern bool quiet;


/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
bool fcfs_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_WAIT;

	/* And append current to waitqueue */
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
void fcfs_release(int resource_id)
{
	struct resource *r = resources + resource_id;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter =
				list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure the waiter  is in the wait status
		 */
		assert(waiter->status == PROCESS_WAIT);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}



#include "sched.h"

/***********************************************************************
 * FIFO scheduler
 ***********************************************************************/
static int fifo_initialize(void)
{
	return 0;
}

static void fifo_finalize(void)
{
}

static struct process *fifo_schedule(void)
{
	struct process *next = NULL;

	/* You may inspect the situation by calling dump_status() at any time */
	 dump_status(); //done 

	/**
	 * When there was no process to run in the previous tick (so does
	 * in the very beginning of the simulation), there will be
	 * no @current process. In this case, pick the next without examining
	 * the current process. Also, when the current process is blocked
	 * while acquiring a resource, @current is (supposed to be) attached
	 * to the waitqueue of the corresponding resource. In this case just
	 * pick the next as well.
	 */
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan) {
		return current;
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue)) {
		/**
		 * If the ready queue is not empty, pick the first process
		 * in the ready queue
		 */
		next = list_first_entry(&readyqueue, struct process, list);

		/**
		 * Detach the process from the ready queue. Note we use list_del_init()
		 * instead of list_del() to maintain the list head tidy. Otherwise,
		 * the framework will complain (assert) on process exit.
		 */
		list_del_init(&next->list);
	}

	/* Return the next process to run */
	return next;
}

struct scheduler fifo_scheduler = {
	.name = "FIFO",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.initialize = fifo_initialize,
	.finalize = fifo_finalize,
	.schedule = fifo_schedule,
};


/***********************************************************************
 * SJF scheduler
 ***********************************************************************/

static struct process *sjf_schedule(void)
{
	/**
	 * Implement your own SJF scheduler here.
	 */
	
    // Declare a pointer to the current task in the run queue
    struct process *curr;

    // Declare a pointer to the task with the shortest expected execution time
    struct process *shortest = NULL;

    // Initialize the minimum expected execution time to the maximum possible value
   unsigned long min_exec_time = ULONG_MAX; 

    // Iterate through the run queue
    list_for_each_entry(curr, &runqueue, run_list)
    {
        // If the current task's expected execution time is less than the current minimum
        if (curr->expected_exec_time < min_exec_time)
        {
            // Update the minimum expected execution time and the pointer to the shortest task
            min_exec_time = curr->expected_exec_time;
            shortest = curr;
        }
    }

    // If a task with the shortest expected execution time was found, return it
    if (shortest)
        return shortest;

    // If no task with the shortest expected execution time was found, return NULL
    return NULL;
} 

struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = sjf_schedule, /* Assign the sjf_schedule() function */
};	
/* done TODO: Assign sjf_schedule() to this function pointer to activate SJF in the system */
};
 


/***********************************************************************
 * SRTF scheduler
 ***********************************************************************/
struct scheduler srtf_scheduler = {
	.name = "Shortest Remaining Time First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	/* You need to check the newly created processes to implement SRTF.
	 * Use @forked() callback to mark newly created processes */
	/* Obviously, you should implement srtf_schedule() and attach it here */
};

static struct process *srtf_schedule(void)
{
    // Declare a pointer to the current task in the run queue
    struct process *curr;
    // Declare a pointer to the task with the shortest remaining execution time
    struct process *shortest = NULL;
    // Initialize the minimum remaining execution time to the maximum possible value
    unsigned long min_remain_time = ULONG_MAX;

    // Iterate through the run queue
    list_for_each_entry(curr, &runqueue, run_list)
    {
        // If the current task's remaining execution time is less than the current minimum
        if (curr->remaining_exec_time < min_remain_time)
        {
            // Update the minimum remaining execution time and the pointer to the shortest task
            min_remain_time = curr->remaining_exec_time;
            shortest = curr;
        }
    }

   // If a task with the shortest expected execution time was found, return it
    if (shortest)
        return shortest;

    // If no task with the shortest expected execution time was found, return NULL
    return NULL;
}
/***********************************************************************
 * Round-robin scheduler
 ***********************************************************************/
struct scheduler rr_scheduler = {
	.name = "Round-Robin",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	/* Obviously, you should implement rr_schedule() and attach it here */
};

static struct process *rr_schedule(void)
{
  // Declare a pointer to the head of the run queue
    struct process *head = list_first_entry(&runqueue, struct process, run_list);
	
    // Declare a pointer to the current task in the run queue
    struct process *curr;

    // If the run queue is empty, return NULL
    if (list_empty(&runqueue))
        return NULL;

    // Move the head of the run queue to the end
    list_del(&head->run_list);
    list_add_tail(&head->run_list, &runqueue);

    // Find the new head of the run queue
    head = list_first_entry(&runqueue, struct process, run_list);

    // Return the new head of the run queue
    return head;
	
};
/***********************************************************************
 * Priority scheduler
 ***********************************************************************/
struct scheduler prio_scheduler = {
	.name = "Priority",
	/**
	 * Implement your own acqure/release function to make priority
	 * scheduler correct.
	 */
	/* Implement your own prio_schedule() and attach it here */
};

static struct process *prio_scheduler(void)
{
// Declare a pointer to the process with the highest priority
    struct process *highest_prio = NULL;
    // Declare a pointer to the current task in the run queue
    struct process *curr;

    // Iterate through the run queue
    list_for_each_entry(curr, &runqueue, run_list)
    {
        // If the current task has a higher priority than the current highest priority task
        if (!highest_prio || curr->priority < highest_prio->priority)
        {
            // Update the highest priority task pointer
            highest_prio = curr;
        }
    }

    // Return the task with the highest priority
    return highest_prio;
	
};

/***********************************************************************
 * Priority scheduler with priority ceiling protocol
 ***********************************************************************/
 struct scheduler pcp_scheduler = {
    .name = "Priority + Priority Ceiling Protocol",
    .acquire = pcp_acquire,
    .release = pcp_release,
    .schedule = pcp_schedule,
/**
	 * Implement your own acqure/release function too to make priority
	 * scheduler correct.
	 */
};

static struct process *pcp_schedule(void)
{
    // Declare a pointer to the process with the highest priority
    struct process *highest_prio = NULL;
    // Declare a pointer to the current task in the run queue
    struct process *curr;

    // Iterate through the run queue
    list_for_each_entry(curr, &runqueue, run_list)
    {
        // If the current task has a higher priority than the current highest priority task
        if (!highest_prio || curr->priority < highest_prio->priority)
        {
            // Update the highest priority task pointer
            highest_prio = curr;
        }
    }

    // Return the task with the highest priority
    return highest_prio;
}

static int pcp_acquire(struct process *p)
{
    // Acquire the lock for the process
    if (acquire_lock(p->lock) != 0)
        return -1;

    // Check if the current priority of the process is higher than the priority ceiling
    if (p->priority < p->lock->ceiling)
    {
        // If so, update the priority of the process to the priority ceiling
        p->priority = p->lock->ceiling;
    }

    // Return success
    return 0;
}

static int pcp_release(struct process *p)
{
    // Release the lock for the process
    release_lock(p->lock);

    // Restore the original priority of the process
    p->priority = p->orig_priority;

    // Return success
    return 0;
}

/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/
struct scheduler pip_scheduler = {
	.name = "Priority + Priority Inheritance Protocol",
	/**
	 * Ditto
	 */
};
