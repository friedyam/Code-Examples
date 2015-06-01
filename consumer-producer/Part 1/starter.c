/*
	Part 1 of Lab 5 for ECE 254.
*/

#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>
#include "queue.c"

//Struct to hold the parameters passed to the producer threads
typedef struct producer_parms{
	int num_producers;
	int pid;
}producer_parms;

//Struct to hold the parameters passed to the consumer threads
typedef struct consumer_parms{
	int num_consumers;
	int cid;
}consumer_parms;

//Total number of messages to send/receive
int total_to_produce;
//Number of messages consumed
int consumer_count;
//Number of messaged produced
int producer_count;

//Producer Semaphore to make sure you don't produce more than the queue size
sem_t producer_sem;
//Consumer Semaphore to stop consuming when the queue is empty
sem_t consumer_sem;
//Mutex to let the queue be accessed by one thread
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
//Mutex to let the producer_count be accessed by one thread
pthread_mutex_t pc_mutex = PTHREAD_MUTEX_INITIALIZER;
//Mutex to let the consumer_count be accessed by one thread
pthread_mutex_t cc_mutex = PTHREAD_MUTEX_INITIALIZER;

//The queue to hold messages between threads
struct Queue *queue;

//Increments the producer_count variable by 1 and returns whether or not
//to continue producing numbers
int continue_producing() {
	int cp = 0;

	pthread_mutex_lock(&pc_mutex);

	if(producer_count < total_to_produce) {
		cp = 1;
		producer_count++;
	}

	pthread_mutex_unlock(&pc_mutex);

	return cp;
}

//Increments the consumer_count variable by 1 and returns whether or not
//to continue consuming numbers
int continue_consuming() {
	int cc = 0;

	pthread_mutex_lock(&cc_mutex);

	if(consumer_count < total_to_produce) {
		cc = 1;
		consumer_count++;
	}

	pthread_mutex_unlock(&cc_mutex);

	return cc;
	
}

//The function that the producer threads run. This function will produce
//numbers based it its id and sends it to the queue
void * producer(void * arg){
	//Holds the parameters sent to this producer thread
 	struct producer_parms params = *((producer_parms*) arg);
 	//Number counter
 	int i = 0;
 	
 	while(1){
 		//Producer produces ints such that i%P == id
 		if(i%params.num_producers == params.pid){
 			//Continue producing if the producers haven't produced the
 			//total number of messages
 			if(continue_producing()) {
 				//Update the producer_sem to let the producers know you are putting
 				//another element in the queue
	 			if(sem_wait(&producer_sem) == -1){
	 				perror("sem_wait failed for Producer");
	 				exit(1);
	 			}
	 			//Lock the queue so no other thread can access it
	 			pthread_mutex_lock(&queue_mutex);
	 			//Send and element to the queue
	 			Enqueue(queue, i);
	 			//Update the conumer_sem to let the consumers know there is
	 			//another element in the queue
	 			if(sem_post(&consumer_sem) == -1){
	 				perror("sem_post failed for Producer");
	 				exit(1);
	 			}
	 			//Unlock the queue so other threads can access it
	 			pthread_mutex_unlock(&queue_mutex);
 			} 
 			//If the producers have produced enough messages break out of the while loop
 			else {
 				break;
 			}
 		}
 		i++;
 	}
 	return 0;
}

//The function that the consumer threads run. This function will consumer
//numbers from the queue an calculate the square root of that number. The
//consumer will print out the number if the square root is whole number.
void * consumer(void * arg){
 	//Holds the parameters sent to this consumer thread
	struct consumer_parms params = *((consumer_parms*) arg);

	//Continue consuming if the consumers haven't consumed the
 	//total number of messages
	while(continue_consuming()){
		//Update the consumer_sem to let the consumers know you are receiving
 		//another element from the queue
		if(sem_wait(&consumer_sem) == -1){
			perror("sem_wait failed for Consumer");
 			exit(1);
		}
		//Lock the queue so no other thread can access it
		pthread_mutex_lock(&queue_mutex);
		//Receive and element from the queue
		int value = Dequeue(queue);
		//Update the producer_sem to let the producers know there is
	 	//another element consumed from the queue
		if(sem_post(&producer_sem) == -1){
			perror("sem_post failed for Consumer");
 			exit(1);
		}
		//Unlock the queue so other threads can access it
		pthread_mutex_unlock(&queue_mutex);
		//Calculate the square root of the int taken out of the queue
		float sqrt_value = sqrt(value);
		//If the square root of the int taken from the queue is a
		//whole number, then print it out
		if((sqrt_value - floor(sqrt_value)) == 0) {
			//Prints (cid, value, sqrt(value))
			printf("%d %d %d\n", params.cid, value, (int) sqrt_value);
		}
	}
	return 0;
 }

//Main program to create the threads/initialize all shared memory
int main(int argc, char *argv[]){
 	//Holds the current time in the system
 	struct timeval tv;
 	//Holds the time before the threads start to be created
 	double time_before_threads;
 	//Holds the time after the threads are all done executing
	double time_after_last_int;
	//Holds the execution time of the consumer and producer threads
	double execution_time;
	//Create a pointer of pthreads for both the producer and consumer
 	pthread_t* producer_array;
 	pthread_t* consumer_array;
 	//Create a pointer to arguments for both the producer and consumer
 	struct producer_parms* producer_args;
 	struct consumer_parms* consumer_args;

 	//Holds the queue size, number of producers to create, number of consumers
 	//to create, and a loop counter, respectively
 	int b, p, c, i;

 	//Must pass 4 parameters to the main program
	if(argc != 5){
 		printf("Usage: ./produce <N> <B> <P> <C>\n");
		printf("<N>: number of integers produced in total. <B>: Buffer size \n");
		printf("<P>: Number of producers. <C>: Number of consumers\n");
		exit(1);
 	}

 	//Initialize the consumer_count and producer_count variables to 0
 	consumer_count = 0;
 	producer_count = 0;
 	//Set variables to number received from the command line
 	total_to_produce = atoi(argv[1]);
 	b = atoi(argv[2]);
	p = atoi(argv[3]);
 	c = atoi(argv[4]);
 	//Create a queue of size b (buffer)
 	queue = createQueue(b);

 	//Initialize consumer and producer thread id arrays and argument arrays
 	producer_array = (pthread_t*) malloc(sizeof(pthread_t)*p);
 	producer_args = (struct producer_parms*) malloc(sizeof(struct producer_parms)*p);
 	consumer_array = (pthread_t*) malloc(sizeof(pthread_t)*c);
 	consumer_args = (struct consumer_parms*) malloc(sizeof(struct consumer_parms)*c);

 	//Initialize two semaphores, one for when the buffer is empty (consumer should block), and one for when the buffer 
 	//reaches the max size of the buffer (producer should block)
 	if (sem_init(&producer_sem, 0, b) != 0){
 		perror("Error creating producer semaphore");
 		exit(1);
 	}
 	if (sem_init(&consumer_sem, 0, 0) != 0){
 		perror("Error creating consumer semaphore");
 		exit(1);
 	}

 	//Get the time before the threads are created
 	gettimeofday(&tv, NULL);
	time_before_threads = tv.tv_sec + tv.tv_usec/1000000.0;

 	//Create a thread for each position in the producer/consumer array 
 	for(i = 0; i < p; i++){
 		producer_args[i].num_producers = p;
 		producer_args[i].pid = i;
 		if(pthread_create(&producer_array[i], NULL, &producer, &(producer_args[i])) != 0){
 			perror("Error creating Producer with ID");
 			exit(1);
 		}
 	} 
 	for(i = 0; i < c; i++){
 		consumer_args[i].num_consumers = c;
 		consumer_args[i].cid = i;
 		if(pthread_create(&consumer_array[i], NULL, &consumer, &(consumer_args[i])) != 0){
			perror("Error creating Consumer with CID");
 			exit(1);
 		}
 	}

 	//These statements block until each producer and consumer has finished
 	//by joining the threads
 	for(i = 0; i < p; i++){
 		pthread_join(producer_array[i], NULL);
 	}
  	for(i = 0; i < c; i++){
 		pthread_join(consumer_array[i], NULL);
 	}

 	//Get the time once the threads are done executing
 	gettimeofday(&tv, NULL);
	time_after_last_int = tv.tv_sec + tv.tv_usec/1000000.0;

	//Calculate how long the program was running and print out the time
	execution_time = time_after_last_int - time_before_threads;
	printf("System execution time: %f seconds\n", execution_time);

	//Call the queue's destructor
	queue_destroy(queue);
	//Free the memory associated with the thread ids and parameters
	free(producer_array);
	free(producer_args);
	free(consumer_array);
	free(consumer_args);
	//Destroy the semaphores
	sem_destroy(&producer_sem);
	sem_destroy(&consumer_sem);
	//Destroy the queue_mutex, producer_count mutex, and consumer_count mutex
	pthread_mutex_destroy(&queue_mutex);
	pthread_mutex_destroy(&pc_mutex);
	pthread_mutex_destroy(&cc_mutex);

	return 0;
 }

 

 

