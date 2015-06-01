/**
 * Consumer for Part 2 of Lab 5
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

//Consumer Process
int main(int argc, char *argv[])
{
	//Holds the queues to receive messages from and the consumer_counter
	mqd_t msg_queue, cc_queue;
	//Holds the queue names to receive messages from and the consumer_counter
	char  *msg_queue_name = NULL;
	char  *cc_queue_name = NULL;
	//Hols the queue mode
	mode_t mode = S_IRUSR | S_IWUSR;
	//Holds the queue attributes
	struct mq_attr attr;
	//The number of messages to receive
	int num_messages;
	//Consumer ID
	int cid;

	//There must be 5 parameters passed to the consumer, else there isn't 
	//enough (or too much) information
	if ( argc !=6 ) {
		printf("The consumer must be given 5 parameters.\n");
		exit(1);
	}

	//Get the data from the parameters passed to consumer and
	//set up the queue attributes
	msg_queue_name = argv[1];
	cc_queue_name = argv[2];
	cid = atoi(argv[3]);
	num_messages = atoi(argv[4]);
	attr.mq_maxmsg  = atoi(argv[5]);
	attr.mq_msgsize = sizeof(int);
	attr.mq_flags   = 0;	/* a blocking queue  */

	//Open up the message queue and make sure that there were no errors
	msg_queue  = mq_open(msg_queue_name, O_RDONLY, mode, &attr);
	if (msg_queue == -1 ) {
		perror("mq_open()");
		exit(1);
	}
	attr.mq_maxmsg  = 1;
	//Open up the consumer counter queue and make sure that there were no errors
	cc_queue  = mq_open(cc_queue_name, O_RDWR, mode, &attr);
	if (cc_queue == -1 ) {
		perror("mq_open()");
		exit(1);
	}

	//Holds the value to consume from the queue and the number of values
	//consumed, respectively
	int value, consumer_count;
	//Holds the square root of the value variable
	float sqrt_value;
	while (1) {
		//Remove the consumer counter from the queue
		if (mq_receive(cc_queue, (char *) &consumer_count, sizeof(int), 0) == -1) {
			perror("mq_receive() failed");
		}
		//If the values consumed is less than the number of values to consume
		//increment consumer_count by 1 and continue consuming messages
		if(consumer_count < num_messages) {
			consumer_count++;
			if (mq_send(cc_queue, (char *) &consumer_count, sizeof(int), 0) == -1) {
				perror("mq_send() failed");
			}
		}
		//Otherwise you have consumed all of the messages and you should break from
		//the loop
		else {
			if (mq_send(cc_queue, (char *) &consumer_count, sizeof(int), 0) == -1) {
				perror("mq_send() failed");
			}
			break;
		}
		//If there is an error in receiving, print that out, otherwise calculate
		//the square root of the value that is consumed
		if (mq_receive(msg_queue, (char *) &value, sizeof(int), 0) == -1) {
			perror("mq_receive() failed");
		} else {
			sqrt_value = sqrt(value);
			//If the square root of the int taken from the queue is a
			//whole number, then print it out
			if((sqrt_value - floor(sqrt_value)) == 0) {
				//Prints (cid, value, sqrt(value))
				printf("%d %d %d\n", cid, value, (int) sqrt_value);
			}
		}
	};

	//Close the message queue and print out any errors that occur
	if (mq_close(msg_queue) == -1) {
		perror("mq_close() failed");
		exit(2);
	}

	//Close the consumer counter queue and print out any errors that occur
	if (mq_close(cc_queue) == -1) {
		perror("mq_close() failed");
		exit(2);
	}

	return 0;
}
