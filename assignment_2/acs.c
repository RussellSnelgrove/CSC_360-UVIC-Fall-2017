/*
CSC 360 Assignment 2
Due: November 6, 2017
Russell Snelgrove



in my code I first wanted to be sure I knew how to use mutexs
I started with one queue and one mutex. 
After seeing that I could make it work I went to try and go to using two.
I could not figure out how to get the code to work for multiple mutexs sharing some kinda of critical section. 
So I then went to semaphores after learning about them in class. 
It made sense to me so I decided to change my code to dealing with semaphores as opposed to mutexs
The next issue I ran into was I had to re-order some of my code because I kept running into errors and 
used stack overflow to trouble shoot some of my code.



To do:

	- wait time	
	- random selection


trouble shooting help from https://stackoverflow.com/questions/1712592/variably-modified-array-at-file-scope
						   http://www.csc.villanova.edu/~mdamian/threads/posixsem.html
						   https://stackoverflow.com/questions/12722904/how-to-use-struct-timeval-to-get-the-execution-time
						   https://www.youtube.com/watch?v=p0SKPpC5r9U
						   
						   
Changes I made to program:

Instead of using two critical sections I used one. Instead of using mutexs I used semaphores.

*/
#include <unistd.h>            
#include <string.h>           
#include <ctype.h>      
#include <stdio.h> 
#include <readline/readline.h>  
#include <stdlib.h>            
#include <sys/types.h>        
#include <sys/wait.h>  
#include <sys/time.h>
#include <pthread.h>
#include <sys/time.h>       
#include <signal.h>     
#include <semaphore.h>
#include <time.h>

       



typedef struct customer {
	int id;							//id
	float arrival_time;				//arriaval_time
	float service_time;				//service_time
	int clerk;						//clerk that is served by
	int queue_number;				// which queue it is entered into 
	long start_time;
	long serve_time;
} customer;


#define maxsize 256					// must use #define or else i get an error
int MAX_INPUT_SIZE = 1024;			//max input size i use when reading in files
customer customers[maxsize];		//array for custonmers
struct timeval start;				//dont forget that this can use microseconds
pthread_t threads[maxsize]; 		// Each thread executes one customer
customer* queue[maxsize];   	    // Stores waiting customers while arrival_time pipe is occupied
int len_queue = 0;					// inital queue used in first attempt at mutexs
int critical_section_in_use = 0;	// checks to see if the critical section is currently used
float waiting_time=0;				// used to calculate the average waiting time in the end of the program
int clerk_1_busy=0;					// Always the first clerk to help the clients and checks to see if the clerk is busy
int clerk_2_busy=0;					// Checks to see if the first clerk is free if not the next defult is the second clerk
int queuerep=0;						//	Used to return which queue was selected for the customer and increments the 
									//	Corrisponding queue
int queue1 =0;						// queue 1
int queue2 =0;						// queue 2
int queue3 =0;						// queue 3
int queue4 =0;						// queue 4

float total_wait=0;


sem_t semaphore;					// my smeaphore



/*
*reads the input file
*/
int readcustomer(char* filePath, char fileContents[MAX_INPUT_SIZE][MAX_INPUT_SIZE]) {
	FILE *fp = fopen(filePath, "r");
	if (fp != NULL) {
		int i = 0;
		while(fgets(fileContents[i], MAX_INPUT_SIZE, fp)) {
			i++;
		}
		fclose(fp);
		return 0;
	} else {
		return 1;
	}
}





/*
* Replaces the colon for making the splitcustomer
* easier to token
*/
void replaceColon(char string[]) {
	int i = 0;
	while (string[i] != '\0') {
		if (string[i] == ':') {
			string[i] = ',';
		}else{
			
		}
		i++;
	}
}



/*
* splits the data into different sections that are then put into the 
* structs attributes 
*/
void splitcustomer(char fileContents[MAX_INPUT_SIZE][MAX_INPUT_SIZE], int numcustomer) {
	int i;
	for (i = 1; i <= numcustomer; i++) {
		replaceColon(fileContents[i]);
		//printf("came back from the simplify stage");
		int j = 0;
		int customerattribute[3];
		char* token = strtok(fileContents[i], ",");
		while (token != NULL) {
			customerattribute[j] = atoi(token);
			token = strtok(NULL, ",");
			j++;
		}

		customer f = {
			customerattribute[0],  // id
			customerattribute[1],  // arrival_Time
			customerattribute[2],  // service_time
		};

		customers [i-1] = f;
	}
}



/*
* Gets the difference in time that is recorded
*/
float getTimeDifference() {
	struct timeval nowTime;
	gettimeofday(&nowTime, NULL);
	long nowMicroseconds = (nowTime.tv_sec * 10 * 100000) + nowTime.tv_usec;
	long startMicroseconds = (start.tv_sec * 10 * 100000) + start.tv_usec;
	return (float)(nowMicroseconds - startMicroseconds) / (10 * 100000);
}


/*
* Gets the difference in time that is recorded
*/
long getnowtime(long r) {
	struct timeval nowTime;
	gettimeofday(&nowTime, NULL);
	r = (nowTime.tv_sec * 10 * 100000) + nowTime.tv_usec;
//	printf("time of day = %ld \n", r);
	return r;
}


/*
*compares the customers and returns the customer that requires to be served first
*/
int customer_compare(customer* first_customer, customer* second_customer) {
	if (first_customer->arrival_time > second_customer->arrival_time) {
		return 1;
		//printf("customer 2 is the first to be served");
	} else if (first_customer->arrival_time < second_customer->arrival_time) {
		return -1;
	}

	else if (first_customer->service_time > second_customer->service_time) {
		return 1;
	} else if (first_customer->service_time < second_customer->service_time) {
		return -1;
	}

	else if (first_customer->id > second_customer->id) {
		return 1;
	} else if (first_customer->id < second_customer->id) {
		return -1;
	}

	else {
		// Should never get here
		printf("Error: failed to sort\n");
		return 0;
	}
}



/*
* Adds the customer to the queue
* "expands" the length of the queue by one
*/
void expand_queue(customer* c) {
	queue[len_queue] = c;
	len_queue = len_queue + 1;
	long r=0;
	c->start_time = getnowtime(r);
}

/*
* stars sorting the overall queue using bubble sort
*/
void sortQueue() {
	int x;
	int y;
	int startingIndex;
	if (critical_section_in_use==1) {
		startingIndex = 1;
	} else if(critical_section_in_use==2){
		startingIndex=2;
	}else{
		startingIndex = 0;
	}
	for (x = startingIndex; x < len_queue; x++) {
		for (y = startingIndex; y < len_queue-1; y++) {
			if (customer_compare(queue[y], queue[y+1]) == 1) {
				customer* temp = queue[y+1];
				//printf("this is workingh for %d", x);
				queue[y+1] = queue[y];
				queue[y] = temp;
			}
		}
	}
}




/*
* Returns a number representing the smallest queue
*/
int find_largest_queue(){
	int max =queue1;
	int v=1;
	if (queue2>max){
		v=2;
		max =queue2;
	}
	if (queue3>max){
		v=3;
		max =queue3;
	}
	if (queue4>max){
		v=4;
		max =queue4;
	}
		
	return v;
}


/*
* Returns a number representing the smallest queue
* or random of the smallest
*/int find_smallest_queue_1(){
	int small[4];
	int trace[4];
	small[0] =queue1;
	trace[0] = 1;
	small[1] =queue2;
	trace[1] = 2;
	small[2] =queue3;	
	trace[2] = 3;
	small[3] =queue4;
	trace[3] = 4;
	int x;
	int y;
	for (x=0; x < 4; x++) {
		for (y = x; y <4; y++) {
			if (small[x]>small[y]) {
				int temp = small[x];
				int temp_1 = trace[x];
				small[x] = small[y];
				trace[x] = trace[y];
				small[y] = temp;
				trace[y] = temp_1;
			}
		}
	}
	
	
	if (small[0]<=small[1]){
		if (small[0]==small[1]){
			int r = rand() % 2;
			srand(time(NULL));  
			if(small[0]==small[2]){
				r = rand() % 3;
				if (small[0]==small[3]){
					r = rand() % 4;
					return trace[r];
				}
				return trace[r];
			}
			return trace[r];
			
		}
		
		
	return trace[0];
	}
	
	
	
	
	return trace[0];
}

/*
* adds one to the queue that recives a member
*/
int addtoqueue(number){
	if (number ==1){
		queue1++;
		return queue1;
	}else if(number ==2){
		queue2++;
		return queue2;
	} else if (number==3){
		queue3++;
		return queue3;
	}else{
		queue4++;
		return queue4;

	}
}



/*
* Returns a number representing the largest queue
* or random of the largest
*/int find_largest_queue_sort(){
	int large[4];
	int trace[4];
	large[0] = queue1;
	trace[0] = 1;
	large[1] =queue2;
	trace[1] = 2;
	large[2] =queue3;	
	trace[2] = 3;
	large[3] =queue4;
	trace[3] = 4;
	int x;
	int y;
	for (x=0; x < 4; x++) {
		for (y = x; y <4; y++) {
			if (large[x]<large[y]) {
				int temp = large[x];
				int temp_1 = trace[x];
				large[x] = large[y];
				trace[x] = trace[y];
				large[y] = temp;
				trace[y] = temp_1;
			}
		}
	}
	
	
	if (large[0]>=large[1]){
		if (large[0]==large[1]){
			int r = rand() % 2;
			srand(time(NULL));  
			if(large[0]==large[2]){
				r = rand() % 3;
				if (large[0]==large[3]){
					r = rand() % 4;
					return trace[r];
				}
				return trace[r];
			}
			return trace[r];
			
		}
		
		
	return trace[0];
	}
	return trace[0];
}
	
int find_queue_length(customer* first_customer) {
	return first_customer->queue_number;
}
	
void sort_length_queue(){
	int queue_number_temp = find_largest_queue();
	int startingIndex;
	if (critical_section_in_use==1) {
		startingIndex = 1;
		return;
	} else if(critical_section_in_use==2){
		startingIndex=2;
	}else{
		startingIndex = 0;
		return;
	}
	int x;
	for (x = startingIndex; x < len_queue; x++) {
		if((find_queue_length(queue[x]))== queue_number_temp ){
			customer* temp = queue[x];
			int k;
			for(k=x;k>startingIndex;k--){
				queue[k+1]=queue[k];
			}
			queue[startingIndex]=temp;
			return;
		}
	}
		
		
		
	
	
}


/*
* Takes the customer and places the into a 
* place in the queue
*/
void get_into_line(customer* c) {

	c->queue_number=find_smallest_queue_1();
	queuerep = addtoqueue(c->queue_number);
	expand_queue(c);
	sortQueue();
	printf("Customer %2d entered the queue ID: %d at length: %d \n", c->id, c->queue_number, queuerep);
	
	

	while ((queue[0]->id != c->id)&&(queue[1]->id != c->id)) {
//		printf("Waiting the id:%d \n", c->id);
		sem_wait(&semaphore);
	}
	
	
	critical_section_in_use ++;

}

/*
* Takes member out the first member of the queue
* After being helped
* Shuffles the remainder of the elements down the queue
*/
void take_out_of_queue() {
	int x = 0;
	while (x < len_queue-1) {
		queue[x] = queue[x+1];
		x = x + 1;
	}
	len_queue = len_queue - 1;
}



/*
* This funcation is where the specific queue
* is subtacted to show that the queue has sent a member to 
* the critical section
*/
int removefromqueu(number){
	if (number ==1){
		queue1--;
		return queue1;
	}else if(number ==2){
		queue2--;
		return queue2;
	} else if (number==3){
		queue3--;
		return queue3;
	}else{
		queue4--;
		return queue4;

	}
}


/*
* This is where we have the customer being taken out of the line after being helped
* I signal that one of the clerks have become free and signal the next customer thread that it is time 
* to enter the critical section
*/
void get_out_of_line(customer* c) {

	if(c->clerk ==1){
		clerk_1_busy=0;
	}else{
		clerk_2_busy=0;
	}
	take_out_of_queue();
	critical_section_in_use--;
	sem_post(&semaphore);

}


/*
* This is the meat of my program
* This is where i get the customer threads and send them
* into the queues to wait until they are to be servered
* we also see the wait times and service times
*/

void* threadFunction(void* customeritem) {
	customer* c = (customer*)customeritem;
	usleep(c->arrival_time * 100000);
	printf("Customer %2d arrives \n", c->id);
	get_into_line(c);
	if (clerk_1_busy==0){
		clerk_1_busy =1;
		c->clerk =1;
	}else{
		clerk_2_busy=2;
		c->clerk =2;
	}
	queuerep = removefromqueu(c->queue_number);	
	printf("A clerk starts serving a customer: start time %.2f sec, the customer ID %2d, the Clerk ID: Clerk %d. \n", getTimeDifference(), c->id,c->clerk);
	long l=0;
	c->serve_time = getnowtime(l);
	usleep(c->service_time * 100000);
//	sort_length_queue();
	printf("A clerk finishes serving a customer: end time %.2f sec, the customer ID %2d, the Clerk ID: Clerk %d. \n", getTimeDifference(),c->id,c->clerk);
	waiting_time= waiting_time+ c->service_time;
	float calculate_wait = (float)((c->serve_time)-(c->start_time)) / (10 * 100000);
	total_wait= total_wait+calculate_wait;
//	sort_length_queue();
	get_out_of_line(c);

	pthread_exit(NULL);
}




/** 
* This is the main function
* In here i call functions to read the input file
* and call functions to deal with those inputs
* I also have a little math before destroy the semaphores and pthrread attr
* The math is to calculate the average waiting time for help from the cleark
*/

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Error: input as ./ACS <customers text file>\n");
		exit(1);
	}

	char fileContents[MAX_INPUT_SIZE][MAX_INPUT_SIZE];
	if (readcustomer(argv[1], fileContents) != 0) {
		printf("Error: Failed to read customers file\n");
		exit(1);
	}
	int number_customers = atoi(fileContents[0]);
	splitcustomer(fileContents, number_customers);

	if(sem_init(&semaphore, 0, 2)==-1){
		printf("Error: failed to initialize conditional variable\n");
		exit(1);
	}
	
	pthread_attr_t attr;
	if (pthread_attr_init(&attr) != 0) {
		printf("Error: failed to initialize attr\n");
		exit(1);
	}
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0) {
		printf("Error: failed to set detachstate\n");
		exit(1);
	}

	gettimeofday(&start, NULL);

	int i;
	for (i = 0; i < number_customers; i++) {
		if (pthread_create(&threads[i], &attr, threadFunction, (void*)&customers[i]) != 0){
			printf("Error: failed to create pthread.\n");
			exit(1);
		}
	}
	for (i = 0; i < number_customers; i++) {
		if (pthread_join(threads[i], NULL) != 0) {
			printf("Error: failed to join pthread.\n");
			exit(1);
		}
	}
	
	float customer_num = (float)number_customers;
	
	float average = (waiting_time/customer_num)/10;
	float waiter = (total_wait/ customer_num);
	printf("The average service time was %.2f seconds\n", average);
	printf("The average wait time was %.2f seconds\n", waiter);

	if (pthread_attr_destroy(&attr) != 0) {
		printf("Error: failed to destroy attr\n");
		exit(1);
	}

    if(sem_destroy(&semaphore)!=0){
		printf("Error: failed to destroy semaphore\n");
		exit(1);
		
	} 
	pthread_exit(NULL);

	return 0;
}
