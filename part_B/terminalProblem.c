#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int no_of_customers;
int no_of_seats; 
int no_of_free_seats; 
int no_of_terminals;
int no_of_free_terms;

//create global user defined thread_ids for customers
typedef struct{
	int customer_id;
	int usage_time;
}customerThreadArg;

//anything you want to add
pthread_cond_t attendantServesCustomer;
pthread_cond_t customerAvailable;
pthread_cond_t terminalAvailable;
pthread_mutex_t mutex;


//functions declaration
void * customer_routine(void *);
void * attendant_routine(void *);



int main(int argc, char ** argv)
{

	//ask user for the total number of customers
	printf("Please enter the total number of customers:\n");
	scanf("%d",&no_of_customers);
	
	
	// ask user to provide the total number of seats & total number of terminals.
	printf("Please enter the total number of seats:\n");
	scanf("%d",&no_of_seats);
	printf("Please enter the total number of terminals:\n");
	scanf("%d",&no_of_terminals);

	//initalise no_of_free_terms and no_of_free_seats
	no_of_free_seats = no_of_seats;
	no_of_free_terms = no_of_terminals;
	
	
	//make variables to store the customers arrival rate and terminal usage time 
	int customers_arrival_rate;
	int terminal_usage_time;

	//make a variable to store the return code from pthread_create() function 
	int rc;

	// ask user to provide the customers arrival rate & terminal usage time.
	printf("Please enter the customers arrival rate:\n");
	scanf("%d",&customers_arrival_rate);
	printf("Please enter the terminal usage time:\n");
	scanf("%d",&terminal_usage_time);

	
	//Initialize mutexes and condition variable objects 
	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&attendantServesCustomer,NULL);
	pthread_cond_init(&customerAvailable,NULL);
	pthread_cond_init(&terminalAvailable,NULL);
		
	//create system defined thread ids for all customers and attendant
	pthread_t threads[no_of_customers+1];
	
	//make an array of customerThreadArg
	customerThreadArg * customerArgs; 
	if((customerArgs = malloc((no_of_customers + 1) * sizeof(customerThreadArg))) == NULL){ 
		fprintf(stderr, "customerArgs out of memory\n");
		exit(1);
	}

	
	//create the attendant thread.
	rc = pthread_create(&threads[0],NULL,attendant_routine,NULL); 
	if(rc){
		printf("ERROR; return code from pthread_create() (attendant) is %d\n",rc);
		exit(-1);
	}
	
	
	//create customer threads according to the arrival rate (in the range between 0 and arrival rate) and 
	//pass user-defined id and terminal usage (in the range between 0 and terminal usage)

	//give random number generator a seed at each iteration of the loop
	srand((unsigned int)time(NULL));

	for(int i = 1; i <= no_of_customers; i++){

		//assign customer id and terminal usage time to customerArgs[i]
		customerArgs[i].customer_id = i;
		customerArgs[i].usage_time = (int)rand() % (terminal_usage_time + 1);

		//wait for a certain amount of time before creating each customer thread
		sleep((int)rand() % (customers_arrival_rate + 1));	

		//create a customer thread and pass the customerArgs struct to each customer
		rc = pthread_create(&threads[i],NULL,customer_routine,&customerArgs[i]); 
		if(rc){
			printf("ERROR; return code from pthread_create() (customer %d) is %d\n", i, rc);
			exit(-1);
		}
	}

	//block this thread until all customer threads have finished their execution
	for(int i = 1; i <= no_of_customers; i++){
		pthread_join(threads[i],NULL);
	}	

	//cancel the attendant's thread once all customers are finished using terminals
	pthread_cancel(threads[0]);
	
    
	//free allocated memory for customerThreadArg struct
	free(customerArgs);	

	//destroy the mutex and condition variable because they're no longer needed
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&attendantServesCustomer);
	pthread_cond_destroy(&customerAvailable);
	pthread_cond_destroy(&terminalAvailable);
	
    pthread_exit(NULL);
}

void * attendant_routine(void * noargs)
{
    while (1) //Continue to serve customers. 
    {
		//The attendant thread must print the following status messages wherever appropriate:
		//"Attendant: The number of free seats is %d. No customers and I'm waiting. \n"
		//"Attendant: The number of free seats now is %d. try to find a free terminal.\n"
		//"Attendant: The number of free terminal s is %d. All terminals are occupied. \n"
		//"Attendant: The number of free terminals is %d. There are free terminals now. \n"
		//"Attendant: Call one customer. The number of free seats is now %d.\n"
		//"Attendant: Assign one terminal to the customer. The number of free terminals is now %d.\n"

    	pthread_mutex_lock(&mutex);

    	//if there are currently no customers, attendant will wait 
    	//for a customer to arrive
		if(no_of_free_seats == no_of_seats){
			printf("Attendant: The number of free seats is %d. No customers and I'm waiting. \n", no_of_free_seats);
			pthread_cond_wait(&customerAvailable,&mutex);
		}

		//try to find a free terminal
		printf("Attendant: The number of free seats now is %d. try to find a free terminal.\n", no_of_free_seats);

		//if all terminals are occupied, wait until a terminal is available
		if(no_of_free_terms == 0){
			printf("Attendant: The number of free terminal s is %d. All terminals are occupied. \n",no_of_free_terms);
			pthread_cond_wait(&terminalAvailable,&mutex);

			//print out a message saying that there are now free terminal(s)
			printf("Attendant: The number of free terminals is %d. There are free terminals now. \n", no_of_free_terms);
		}

		//pick one customer who's sitting and tell them that the attendant is ready to serve them
		no_of_free_seats++;
		printf("Attendant: Call one customer. The number of free seats is now %d.\n", no_of_free_seats);

		//tell the customer that the attendant is ready to serve them
		pthread_cond_signal(&attendantServesCustomer);
		pthread_mutex_unlock(&mutex);

		//assign one terminal to the customer
		pthread_mutex_lock(&mutex);
		no_of_free_terms--;
		printf("Attendant: Assign one terminal to the customer. The number of free terminals is now %d.\n", no_of_free_terms);
		pthread_mutex_unlock(&mutex);
    }
}

void * customer_routine(void * args)
{
	//customer thread must print the following status messages wherever appropriate where id is the identifying number of the thread:
	//"Customer %d arrives.\n"
	//"Customer %d: oh no! all seats have been taken and I'll leave now!\n" 
	//"Customer %d: I'm lucky to get a free seat from %d.\n"
	//"Customer %d: I'm to be served.\n"
	//"Customer %d: I'm getting a terminal now.\n"
	//"Customer %d: I'm finished using the terminal and leaving.\n"

	//get the customer id and customer terminal usage time
	int customer_id = ((customerThreadArg*)args)->customer_id;
	int term_usage_time = ((customerThreadArg*)args)->usage_time;

	//a customer just arrives 
	printf("Customer %d arrives\n", customer_id);

	pthread_mutex_lock(&mutex);
	if(no_of_free_seats == 0){
		//if there's no seat available then customer will leave
		printf("Customer %d: oh no! all seats have been taken and I'll leave now!\n", customer_id);
		pthread_mutex_unlock(&mutex);

	}else{

		//the customer occupies a seat
		printf("Customer %d: I'm lucky to get a free seat from %d\n",customer_id, no_of_free_seats);
		no_of_free_seats--;

		//wake up the attendant if they're asleep
		pthread_cond_signal(&customerAvailable);

		//wait until an attendant serves customer
		printf("Customer %d: I'm waiting to be served\n", customer_id);
		pthread_cond_wait(&attendantServesCustomer,&mutex);

		//Customer is getting a terminal
		printf("Customer %d: I'm getting a terminal now.\n", customer_id);

		//customer is now assigned a terminal and is using it
		//for a period of time
		pthread_mutex_unlock(&mutex);
		sleep(term_usage_time);

		//customer is done with the terminal and leaves
		pthread_mutex_lock(&mutex);
		no_of_free_terms++;
		printf("Customer %d: I'm finished using the terminal and leaving.\n", customer_id);

		pthread_cond_signal(&terminalAvailable);
		pthread_mutex_unlock(&mutex);

	}
	pthread_exit(NULL);

}