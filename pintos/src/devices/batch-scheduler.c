/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "lib/random.h" //generate random numbers
#include <string.h>
#include "devices/timer.h"
#include <list.h>
#include <unistd.h>

#define BUS_CAPACITY 3
#define SENDER 0
#define RECEIVER 1
#define NORMAL 0
#define HIGH 1

/*
 *	initialize task with direction and priority
 *	call o
 * */
typedef struct {
	int direction;
	int priority;
} task_t;

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive, unsigned int num_priority_send, unsigned int num_priority_receive);

void senderTask(void *);
void receiverTask(void *);
void senderPriorityTask(void *);
void receiverPriorityTask(void *);


void oneTask(task_t task);/*Task requires to use the bus and executes methods below*/
void getSlot(task_t task); /* task tries to use slot on the bus */
void transferData(task_t task); /* task processes data on the bus either sending or receiving based on the direction*/
void leaveSlot(task_t task); /* task release the slot */

// Global variables and semaphores
struct condition cond_norm_send;
struct condition cond_norm_recv;
struct condition cond_prio_send;
struct condition cond_prio_recv;
struct lock lock;
int TASKS_ON_BUS = 0;
int DIRECTION = 0;

/* initializes semaphores */ 
void init_bus(void){ 

	// Initate semaphores and locks 
	random_init((unsigned int)123456789); 
	cond_init(&cond_norm_send);
	cond_init(&cond_norm_recv);
	cond_init(&cond_prio_send);
	cond_init(&cond_prio_recv);

	lock_init(&lock);

}

/*
 *  Creates a memory bus sub-system  with num_tasks_send + num_priority_send
 *  sending data to the accelerator and num_task_receive + num_priority_receive tasks
 *  reading data/results from the accelerator.
 *
 *  Every task is represented by its own thread. 
 *  Task requires and gets slot on bus system (1)
 *  process data and the bus (2)
 *  Leave the bus (3).
 */

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        			unsigned int num_priority_send, unsigned int num_priority_receive) {

	// For each task that comes we will loop and create a thread per task.
	unsigned int i;
	for(i = 0; i <= num_tasks_send; i++) {
		thread_create("Sender", NORMAL, senderTask, NULL);
	}
	for(i = 0; i <= num_task_receive; i++) {
    	thread_create("Receiver", NORMAL, receiverTask, NULL);
	}
	for(i = 0; i <= num_priority_send; i++) {
        thread_create("Priority sender", HIGH, senderPriorityTask, NULL);
	}
	for(i = 0; i <= num_priority_receive; i++) {
        thread_create("Priority receiver", HIGH, receiverPriorityTask, NULL);
	}
}

/* Normal task,  sending data to the accelerator */
void senderTask(void *aux UNUSED){
        task_t task = {SENDER, NORMAL};
        oneTask(task);
}

/* High priority task, sending data to the accelerator */
void senderPriorityTask(void *aux UNUSED){
        task_t task = {SENDER, HIGH};
        oneTask(task);
}

/* Normal task, reading data from the accelerator */
void receiverTask(void *aux UNUSED){
        task_t task = {RECEIVER, NORMAL};
        oneTask(task);
}

/* High priority task, reading data from the accelerator */
void receiverPriorityTask(void *aux UNUSED){
        task_t task = {RECEIVER, HIGH};
        oneTask(task);
}

/* abstract task execution*/
void oneTask(task_t task) {
  getSlot(task);
  transferData(task);
  leaveSlot(task);
}


/* task tries to get slot on the bus subsystem */
void getSlot(task_t task) {

	// Lock the bus
	lock_acquire(&lock);
	
	// If the bus is full and the direction is wrong we must wait, i.e adding to a queue.
	if(TASKS_ON_BUS == 3 || task.direction != DIRECTION && TASKS_ON_BUS > 0) {
		
		// If my prio is high and I am a sender, add me to this queue.
		if(task.priority && task.direction == SENDER) {
			cond_wait(&cond_prio_send, &lock);
		}
		// If my prio is high and I am a receiver, add me to this queue.
		else if (task.priority && task.direction == RECEIVER) {
			cond_wait(&cond_prio_recv, &lock);
		}
		// If my prio is normal and I am a sender, add me to this queue.
		else if(!task.priority && task.direction == SENDER) {
			cond_wait(&cond_norm_send, &lock);
		}
		// Else I must be a normal and a receving task, add me to this queue.
		else {
			cond_wait(&cond_norm_recv, &lock);
		}
	}
	
	// If there is a least one slot available on the bus and the direction
	// is same as mine.	
	TASKS_ON_BUS++;
	DIRECTION = task.direction;

	// Release lock
	lock_release(&lock);
	
}
/* task processes data on the bus send/receive */
void transferData(task_t task) {
	
	// Just a random sleep function that sleeps for some time between 0-10
	timer_sleep((int64_t) random_ulong() % 10);
}	

/* task releases the slot */
void leaveSlot(task_t task) {
	// Lock and make space on the bus
	lock_acquire(&lock);
	TASKS_ON_BUS--;
/*
 * The following code will check wheather you are a sender or receiver
 * Depending on that, you first want to let every high prio task in your direction
 * have the bus. If there is no high prio task, then you can signal to the normal tasks
 * in the same direction of yours.
 * If there is no high or normal process in your direction, start signaling to high 
 * prio processes in the other direction and if there are no such tasks keep trying with
 * the normal processes.
 * */	
	
	if(task.direction == SENDER) {
		if(!list_empty(&cond_prio_send.waiters)) {
			cond_signal(&cond_prio_send, &lock);
		}
		else if(!list_empty(&cond_prio_send.waiters)) {
			cond_signal(&cond_norm_send, &lock);
		}
		else if(!list_empty(&cond_prio_recv.waiters)) {
        	cond_signal(&cond_prio_recv, &lock);
         }
		else if(!list_empty(&cond_norm_recv.waiters)) {
			cond_signal(&cond_norm_recv, &lock);
		}
	}
	else {
		if(!list_empty(&cond_prio_recv.waiters)) {
             cond_signal(&cond_prio_recv, &lock);
		}
		else if(!list_empty(&cond_norm_recv.waiters)) {
             cond_signal(&cond_norm_recv, &lock);
		}
		else if(!list_empty(&cond_prio_recv.waiters)) {
			cond_signal(&cond_prio_recv, &lock);
		}
		else if(!list_empty(&cond_norm_recv.waiters)) {
			cond_signal(&cond_norm_recv, &lock);
		}
	}
	lock_release(&lock);
}
