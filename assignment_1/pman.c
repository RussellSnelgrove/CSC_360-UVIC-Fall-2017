
#include <unistd.h>            
#include <string.h>           
#include <ctype.h>      
#include <stdio.h> 
#include <readline/readline.h>  
#include <stdlib.h>            
#include <sys/types.h>        
#include <sys/wait.h>         
#include <signal.h>            


int curr_run = 1;
int not_run = 0;
int MAX = 128;

/*some things I looked at:
http://www2.cs.uidaho.edu/~krings/CS270/Notes.S10/270-F10-25.pdf
https://stackoverflow.com/questions/17292545/how-to-check-if-the-input-is-a-number-or-not-in-c
*/

typedef struct node_a {
	pid_t pid;
	char* name_process;
	int running;
	int number_processes;
	struct node_a* next;
} node_a;

node_a* listHead = NULL;


/*checks to see if the pid exists in this program*/
int exists(pid_t pid) {
	node_a* check = listHead;
	while (check != NULL) {
		if (check->pid == pid) {
			return curr_run;
		}
		check = check->next;
	}
	return not_run;
}


/* gets the user input, used the same function i used in seng 265 */
int getUserInput(char** input_str) {
	char* rawInput = readline("PMan: > ");
	if (strcmp(rawInput, "") == 0) {
		return not_run;
	}
	char* token = strtok(rawInput, " ");
	int i;
	for (i = 0; i < MAX; i++) {
		input_str[i] = token;
		token = strtok(NULL, " ");
	}
	return curr_run;
}


/*reads in the file*/
void file_reader(char* path, char** contents) {
	FILE *fp = fopen(path, "r");
	int i=0;
	char filestream[1024];
	if (fp != NULL) {
		while (fgets(filestream, sizeof(filestream)-1, fp) != NULL) {
			char* token;
			token = strtok(filestream, " ");
			contents[i] = token;
			while (token != NULL) {
				contents[i] = token;
				token = strtok(NULL, " ");
				i++;
			}
			//for(int k=0;k<3;k++)
		}
		fclose(fp);
	} else {
		printf("Error: The stat file could not be read \n");
	}
}

/*adds to the linked list, used from csc 111 code*/
void add_list(pid_t pid, char* name_process) {
	//printf("at the add_list function");
	node_a* n = (node_a*)malloc(sizeof(node_a));
	n->name_process = name_process;
	n->running = curr_run;
	n->pid = pid;
	n->next = NULL;
	if (listHead == NULL) {
		listHead = n;
	} else {
		node_a* check = listHead;
		while (check->next != NULL) {
			check = check->next;
		}
		check->next = n;
	}
}

/* deletes the program from the linked list*/
void remove_list(pid_t pid) {
	if (!exists(pid)) {
		return;
	}

	node_a* cycle_1 = NULL;
	node_a* cycle = listHead;
	while (cycle != NULL) {
		if (cycle->pid == pid) {
			if (cycle == listHead) {
				listHead = listHead->next;
			} else {
				cycle_1->next = cycle->next;
			}
			free(cycle);
			return;
		}
		cycle_1 = cycle;
		cycle = cycle->next;
	}
}

/* gets information about the node */
node_a* get_node(pid_t pid) {
	node_a* check = listHead;
	while (check != NULL) {
		if (check->pid == pid) {
			return check;
		}
		check = check->next;
	}
	return NULL;
}

/* Runs a program that is in the input ex. < bg ./inf hello 5 > */
void bg(char** input_str) {
	pid_t pid = fork();
	if (pid == 0) {
		char* command = input_str[1];
		execvp(command, &input_str[1]);
		printf("Error: could not execute the command \n");
		exit(1);
	} else if (pid > 0) {
		printf("Started background name_process %d\n", pid);
		add_list(pid, input_str[1]);
		sleep(1);
	} else {
		printf("Error: failed to fork\n");
	}
}

/* kills a process that is running, ex . < bgkill 21665 >*/
void bgkill(pid_t pid) {
/*	printf("%d\n", pid);
	if(!isdigit(pid)){
		printf("The pid entered was not a number \n");
			
		return;
	}
*/
	if (!exists(pid)) {
		printf("Error: pid does not exist \n");
		return;
	}
	int error = kill(pid, SIGTERM);
	if (!error) {
		sleep(1);
	} else {
		printf("Error: failed to run bgkill\n");
	}
}

/* stops a running proccess, ex . < bgstop 21665 > */
void bgstop(pid_t pid) {
	if(!isdigit(pid)){
		printf("The pid is not a number");
		return;
	}if (!exists(pid)) {
		printf("Error: pid does not exist \n");
		return;
	}
	int error = kill(pid, SIGSTOP);
	if (!error) {
		sleep(1);
	} else {
		printf("Error: failed to run bgstop\n");
	}
}

/* restarts a stopped proccess, ex. < bgstart 21665 > */
void bgstart(pid_t pid) {
	if(!isdigit(pid)){
		printf("The pid entered was not a number\n");
		return;
	}
	if (!exists(pid)) {
		printf("Error: pid does not exist \n");
		return;
	}
	int error = kill(pid, SIGCONT);
	if (!error) {
		sleep(1);
	} else {
		printf("Error: failed to execute bgstart\n");
	}
}

/* Shows a list of processes in the linked list, ex. < bglist() >*/
void bglist() {
	node_a* check = listHead;
	int counter = 0;
	while (check != NULL) {
		counter++;
		char* stopped = "";
		if (!check->running) {
			stopped = "(stopped)";
		}
		printf("%d:   %s %s\n", check->pid, check->name_process, stopped);
		check = check->next;
	}
	printf("Total background jobs:\t%d\n", counter);
}

/* list the information about the pid, ex. < pstat 21665 >*/
void pstat(pid_t pid) {
	if(!isdigit(pid)){
		printf("The entered pid is not a number \n");
		return;
	}
	if (exists(pid)) {
		char path_1[MAX];
		char path_2[MAX];
		sprintf(path_1, "/proc/%d/stat", pid);
		sprintf(path_2, "/proc/%d/status", pid);

		char* contents[MAX];
		file_reader(path_1, contents);

		char contents_1[MAX][MAX];
		FILE* statusFile = fopen(path_2, "r");
		if (statusFile != NULL) {
			int i = 0;
			while (fgets(contents_1[i], MAX, statusFile) != NULL) {
				i++;
			}
			fclose(statusFile);
		} else {
			printf("Error: could not read status file\n");
			return;
		}

		char* p;
		long unsigned int utime = strtoul(contents[13], &p, 10) / sysconf(_SC_CLK_TCK);
		long unsigned int stime = strtoul(contents[14], &p, 10) / sysconf(_SC_CLK_TCK);
		char* switch_2 = contents_1[39];
		char* switch_1 = contents_1[40];

//		printf
		printf("comm:    %s\n", contents[1]);
		printf("state:   %s\n", contents[2]);
		printf("utime:   %lu\n", utime);
		printf("stime:   %lu\n", stime);
		printf("rss:     %s\n", contents[24]);
		printf("%s", switch_2);
		printf("%s", switch_1);
	} else {
		printf("Error: Process %d does not exist.\n", pid);
	}
}




/*	updates linked_list*/
void updateProcessStatuses() {
	pid_t pid;
	int	status;
	while (curr_run) {
		pid = waitpid(-1, &status, WCONTINUED | WNOHANG | WUNTRACED);
		//printf("%d\n",curr_run);
		if (pid > 0) {
			if (WIFEXITED(status)) {
				printf("Background pid = %d terminated.\n", pid);
				remove_list(pid);
			}if (WIFSTOPPED(status)) {
				printf("Background pid = %d was stopped.\n", pid);
				node_a* n = get_node(pid);
				n->running = not_run;
			}if (WIFCONTINUED(status)) {
				printf("Background pid =  %d was started.\n", pid);
				node_a* n = get_node(pid);
				n->running = curr_run;
			}if (WIFSIGNALED(status)) {
				printf("Background pid = %d was killed.\n", pid);
				remove_list(pid);
			}
		} else {
			//printf("successful break");
			break;
		}
	}
}


int main() {
	while (curr_run) {
		char* input_str[MAX];
		getUserInput(input_str);
		updateProcessStatuses();
		if(strcmp(input_str[0], "bg")==0){
			if(input_str[1] == NULL){
				printf("Cannot run that command \n");
			}
			bg(input_str);
			
		}else if(strcmp(input_str[0], "bgkill")==0){
			if(input_str[1] == NULL){
				printf("Cannnot run that command \n ");
			}
			pid_t pid = atoi(input_str[1]);
				if(pid!=0){
				bgkill(pid);
			}
		}else if(strcmp(input_str[0], "bglist")==0){

			bglist();
	
		}else if(strcmp(input_str[0], "bgstop")==0){
			if(input_str[1]==NULL){
				printf("Cannot run that command \n");

			}	
			pid_t pid = atoi(input_str[1]);
			bgstop(pid);

		}else if(strcmp(input_str[0], "bgstart")==0){
			if(input_str[1]==NULL){
			}
			pid_t pid=atoi(input_str[1]);
			bgstart(pid);
		}else if(strcmp(input_str[0], "pstat")==0){
			if(input_str[1]==NULL){
			}
			pid_t pid=atoi(input_str[1]);
			pstat(pid);
		}else if(strcmp(input_str[0], "exit")==0){
			break;	
		}else if (strcmp(input_str[0],"")==0){
		}else{
			printf("That is an invalid command. \n");
		}
		updateProcessStatuses();
	}

	exit(0);
}

