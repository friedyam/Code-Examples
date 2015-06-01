/**
 * Producer for Part 2 of Lab 5
 */

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <signal.h>
#include <math.h>

#define _XOPEN_SOURCE 600

//Producer Process
int main(int argc, char *argv[])
{
	//Holds the queues to send messages from and the producer_counter
	mqd_t msg_queue, pc_queue;
	//Holds the queue names to send messages from and the producer_counter
	char  *msg_queue_name = NULL;
	char  *pc_queue_name = NULL;
	//Hols the queue mode
	mode_t mode = S_IRUSR | S_IWUSR;
	//Holds the queue attributes
	struct mq_attr attr;
	//The number of messages to receive
	int num_messages;
	//Holds the number of producer and the producer id, respectively
	int num_producers, pid;

	//There must be 6 parameters passed to the producer, else there isn't 
	//enough (or too much) information
	if ( argc !=7 ) {
		printf("The receiver must be given 6 parameters.\n");
		exit(1);
	}

	//Get the data from the parameters passed to producer and
	//set up the queue attributes
	msg_queue_name = argv[1];
	pc_queue_name = argv[2];
	pid = atoi(argv[3]);
	num_producers = atoi(argv[4]);
	num_messages = atoi(argv[5]);
	attr.mq_maxmsg  = atoi(argv[6]);
	attr.mq_msgsize = sizeof(int);
	attr.mq_flags   = 0;	/* a blocking queue  */
	
	//Open up the message queue and make sure that there were no errors
	msg_queue  = mq_open(msg_queue_name, O_RDWR, mode, &attr);
	if (msg_queue == -1 ) {
		perror("mq_open()");
		exit(1);
	}

	attr.mq_maxmsg  = 1;
	//Open up the producer counter queue and make sure that there were no errors
	pc_queue  = mq_open(pc_queue_name, O_RDWR, mode, &attr);
	if (pc_queue == -1 ) {
		perror("mq_open()");
		exit(1);
	}

	//Loop counter and the number of messages to produce, respectively
	int i, producer_count;
	i = 0;
	//Send the number of messages to the queue specified by num_messages
	while (1) {
		//If the remainder of i divided by the number of producers is equal
		//to the producer id, send that number to the queue
		if(i%num_producers == pid) {
			//Remove the producer counter from the queue
			if (mq_receive(pc_queue, (char *) &producer_count, sizeof(int), 0) == -1) {
				perror("mq_receive() failed");
			}
			//If the values produced is less than the number of values to produce
			//increment producer_count by 1 and continue producing messages
			if(producer_count < num_messages) {
				producer_count++;
				if (mq_send(pc_queue, (char *) &producer_count, sizeof(int), 0) == -1) {
					perror("mq_send() failed");
				}
			}
			//Otherwise you have produced all of the messages and you should break from
			//the loop
			else {
				if (mq_send(pc_queue, (char *) &producer_count, sizeof(int), 0) == -1) {
					perror("mq_send() failed");
				}
				break;
			}
			//Send the value to the message queue
			if (mq_send(msg_queue, (char *) &i, sizeof(int), 0) == -1) {
				perror("mq_send() failed");
			}
		}
		i++;
	}

	//Close the message queue and print out any errors that occur
	if (mq_close(msg_queue) == -1) {
		perror("mq_close() failed");
		exit(2);
	}
	//Close the producer counter queue and print out any errors that occur
	if (mq_close(pc_queue) == -1) {
		perror("mq_close() failed");
		exit(2);
	}

	return 0;
}
