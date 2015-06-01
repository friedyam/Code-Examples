/*
	Main function for part 2 of Lab 5
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[])
{
	//Queue names
	char* msg_queue_name = "/mailbox1_kris";
	char* pc_queue_name = "/mailbox2_kris";
	char* cc_queue_name = "/mailbox3_kris";

	//Holds the queues to send messages to
	mqd_t msg_queue, pc_queue, cc_queue;
	//Holds the the child process pid
	pid_t child_pid;
	//Holds the current time in the system
	struct timeval tv;
	//Holds the time before the fork
	double time_before_fork;
	//Holds the time after the last message is received
	double time_after_last_int;

	//Time taken to transmit the data among processes
	double execution_time;

	//Used to wait for the child process to return/end
	int child_status;

	//Loop counter, number of producers to create, number of consumers
	//to create and producer and consumer counter initial value, respectively
	int i, p, c, counter_init = 0;

	//Holds the id of a consumer or producer
	char id[4];

	//Setup the argument list for the producer child process.
	char* producer_arg_list[] = {
    	"./producer.out",	/* argv[0], the name of the program.  */
    	msg_queue_name,
    	pc_queue_name,
    	"0",
    	argv[3],     
    	argv[1], 
    	argv[2],
    	NULL      /* The argument list must end with a NULL.  */
  	};

  	//Setup the argument list for the consumer child process.
  	char* consumer_arg_list[] = {
    	"./consumer.out",	/* argv[0], the name of the program.  */
    	msg_queue_name,
    	cc_queue_name,
    	"0",     
    	argv[1], 
    	argv[2],
    	NULL      /* The argument list must end with a NULL.  */
  	};

  	//Setup up the mode for the queueS to use
	mode_t mode = S_IRUSR | S_IWUSR;
	//Setup the attributes for the queueS to use
	struct mq_attr attr;

	//Must pass 4 parameters to the main program
	if(argc != 5){
 		printf("Usage: ./produce <N> <B> <P> <C>\n");
		printf("<N>: number of integers produced in total. <B>: Buffer size \n");
		printf("<P>: Number of producers. <C>: Number of consumers\n");
		exit(1);
 	}

 	p = atoi(argv[3]);
 	c = atoi(argv[4]);

	//Create queue attributes.
	attr.mq_maxmsg  = atoi(argv[2]);
	attr.mq_msgsize = sizeof(int);
	attr.mq_flags   = 0;		/* a blocking queue  */

	//Open up the queue to send messages to. If opening up the
	//queue fails, print out a message
	msg_queue  = mq_open(msg_queue_name, O_RDWR | O_CREAT, mode, &attr);
	if (msg_queue == -1 ) {
		perror("mq_open() failed");
		exit(1);
	}

	attr.mq_maxmsg = 1;
	//Open up the queue to hold the producer counter. If opening up the
	//queue fails, print out a message
	pc_queue  = mq_open(pc_queue_name, O_RDWR | O_CREAT, mode, &attr);
	if (pc_queue == -1 ) {
		perror("mq_open() failed");
		exit(1);
	}
	//Open up the queue to hold the consumer counter. If opening up the
	//queue fails, print out a message
	cc_queue  = mq_open(cc_queue_name, O_RDWR | O_CREAT, mode, &attr);
	if (cc_queue == -1 ) {
		perror("mq_open() failed");
		exit(1);
	}

	//Send the initial counter value to the producer and consumer counter queues
	if (mq_send(pc_queue, (char *) &counter_init, sizeof(int), 0) == -1) {
		perror("mq_send() failed");
	}
	if (mq_send(cc_queue, (char *) &counter_init, sizeof(int), 0) == -1) {
		perror("mq_send() failed");
	}

	//Get the time before the child processes are created
	gettimeofday(&tv, NULL);
	time_before_fork = tv.tv_sec + tv.tv_usec/1000000.0;

	//Create all of the producer child processes
	for(i = 0; i < p; i++) {
		//Convert the producer id from an int to a char array
		sprintf(id, "%d", i);
		producer_arg_list[3] = id;
		child_pid = fork ();
	  	if (child_pid != 0) {
	  		/* This is the parent process.  */
	    	//DO NOTHING!
	  	}
	  	else {
	    	/* Now execute ./producer.out, searching for it in the path.  */
	    	execvp ("./producer.out", producer_arg_list);
	    	/* The execvp function returns only if an error occurs.  */
	    	fprintf (stderr, "an error occurred in execvp\n");
	    	abort ();
	  	}
	}

	//Create all of the consumer child processes
	for(i = 0; i < c; i++) {
		//Convert the consumer id from an int to a char array
		sprintf(id, "%d", i);
		consumer_arg_list[3] = id;
		child_pid = fork ();
	  	if (child_pid != 0) {
	  		/* This is the parent process.  */
	    	//DO NOTHING!
	  	}
	  	else {
	    	/* Now execute ./consumer.out, searching for it in the path.  */
	    	execvp ("./consumer.out", consumer_arg_list);
	    	/* The execvp function returns only if an error occurs.  */
	    	fprintf (stderr, "an error occurred in execvp\n");
	    	abort ();
	  	}
	}

	//Wait for all of the child processes to finish executing and return
	for(i = 0; i < (c + p); i ++) {
		wait(&child_status);
		if(WIFEXITED(child_status)){}
		//Exited abnormally
		else {
			perror("Child process exited abnormally");
			fflush(stderr);
			exit(EXIT_FAILURE);
		}
	}

	//Get the time after last message is received
	gettimeofday(&tv, NULL);
	time_after_last_int = tv.tv_sec + tv.tv_usec/1000000.0;

	//Close the queues.
	if (mq_close(msg_queue) == -1) {
		perror("mq_close() failed");
		exit(2);
	}
	if (mq_close(pc_queue) == -1) {
		perror("mq_close() failed");
		exit(2);
	}
	if (mq_close(cc_queue) == -1) {
		perror("mq_close() failed");
		exit(2);
	}
	//Destroy the queues and free up the resources associated with it.
	if (mq_unlink(msg_queue_name) != 0) {
		perror("mq_unlink() failed");
		exit(3);
	}
	if (mq_unlink(pc_queue_name) != 0) {
		perror("mq_unlink() failed");
		exit(3);
	}
	if (mq_unlink(cc_queue_name) != 0) {
		perror("mq_unlink() failed");
		exit(3);
	}

	//Calculate the child process execution time
	execution_time = time_after_last_int - time_before_fork;

	//Print out timing data
	printf("Time to transmit data: %f\n", execution_time);

	return 0;
}